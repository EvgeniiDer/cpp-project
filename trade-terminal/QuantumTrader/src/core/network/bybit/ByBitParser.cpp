#include"ByBitParser.h"
#include"../../../external/simdjson/simdjson.h"
#include<QDebug>
#include<QDateTime>
#include<QStringList>
#include<charconv>
#include<algorithm>
#include<string_view>


//Вунтренние хелперы

namespace
{
	double fastParseDouble(std::string_view sv)
	{
		// std::from_chars_result - структура, которую возвращает функция std::from_chars.
		// При успешном парсинге число сохраняется в переменную value (третий аргумент),
		// а поле result.ec получает значение std::errc() (означает "нет ошибки").
		// При ошибке (не число или выход за диапазон) value не изменяется,
		// а result.ec будет содержать код ошибки (например, std::errc::invalid_argument).
		double value = 0.0;
		std::from_chars_result result = std::from_chars(sv.data(), sv.data() + sv.size(), value);
		return (result.ec == std::errc()) ? value : 0.0;

	}
	
	int64_t fastParseInt64(std::string_view sv)
	{
		int64_t value = 0;
		std::from_chars_result result = std::from_chars(sv.data(), sv.data() + sv.size(), value);
		return(result.ec == std::errc()) ? value : 0;
	}
}

//==REST===============================================================================================================
int ByBitParser::parserPingResponse(const QByteArray& rawData, QString& outErrorMsg)
{
	simdjson::dom::parser parser;
	try
	{
		// Приводим const char* к const uint8_t* (требование simdjson).
		// Данные не меняются, только тип указателя.
		// Обрати внимание: символы и цифры уже имеют числовой аналог (байты),
		// поэтому simdjson сохраняет весь документ как набор чисел (байт). Подсказка что бы не забть (int)'A' == 65 если не путаю по Аски
		simdjson::dom::element root = parser.parse(reinterpret_cast<const uint8_t*>(rawData.constData()), rawData.size());
		
		int64_t retCode = root["retCode"].get_int64().value();
		if (retCode != 0)
		{
			std::string_view retMsg = root["retMsg"].get_string().value();
			outErrorMsg = QString::fromUtf8(retMsg.data(), retMsg.size());
		}
		return static_cast<int>(retCode);
	}
	catch (const simdjson::simdjson_error& er)
	{
		outErrorMsg = "simdjson Error: " + QString::fromUtf8(er.what());
		return -1;
	}
}

std::vector<Candle> ByBitParser::parseHistory(const QByteArray& rawData)
{
	std::vector<Candle> chunk;
	simdjson::ondemand::parser parser;
	simdjson::padded_string json(rawData.constData(), rawData.size());

	try
	{
		simdjson::ondemand::document doc = parser.iterate(json);

		int64_t retCode = doc["retCode"].get_int64();
		if (retCode != 0)return chunk;
		
		simdjson::ondemand::array list = doc["result"]["list"].get_array();
		chunk.reserve(1000);
		for (simdjson::ondemand::value raw : list)
		{
			simdjson::ondemand::array candle_array = raw.get_array();
			simdjson::ondemand::array_iterator it = candle_array.begin();

			Candle candle;
			candle.timestamp = fastParseInt64((*it).get_string().value()) / 1000; ++it;
			candle.open = fastParseDouble((*it).get_string().value());        ++it;
			candle.high = fastParseDouble((*it).get_string().value());        ++it;
			candle.low = fastParseDouble((*it).get_string().value());        ++it;
			candle.close = fastParseDouble((*it).get_string().value());        ++it;
			candle.volume = fastParseDouble((*it).get_string().value());

			chunk.push_back(candle);
		}
	}
	catch (const simdjson::simdjson_error& er)
	{
		qDebug() << "[ByBitParser] Critical error parsing history:" << er.what();
		return chunk;
	}
	std::reverse(chunk.begin(), chunk.end());
	return chunk;
}

QStringList ByBitParser::parseAvailableSymbols(const QByteArray& rawData)
{
	QStringList symbols;
	simdjson::dom::parser parser;
	try
	{
		simdjson::dom::element root = parser.parse(reinterpret_cast<const uint8_t*>(rawData.constData()), rawData.size());

		int64_t retCode = root["retCode"].get_int64().value();
		if (retCode != 0) return symbols;

		simdjson::dom::array array_list = root["result"]["list"].get_array().value();
		symbols.reserve(static_cast<int>(array_list.size()));

		for (simdjson::dom::element item : array_list)
		{
			std::string_view str_view = item["symbol"].get_string().value();
			symbols.append(QString::fromUtf8(str_view.data(), str_view.size()));
		}
		symbols.sort();
	}catch (const simdjson::simdjson_error& er)
	{
		qDebug() << "[ByBitParser] Error parsing available symbols:" << er.what();
	}
	return symbols;
}

//==WebSocket incoming =====================================================================
std::optional<Candle> ByBitParser::parseLiveCandle(const QByteArray& jsonMessage, QString& outSymbol, ChartInterval& outInterval)
{
	simdjson::dom::parser parser;
	try
	{
		simdjson::dom::element root = parser.parse(
			reinterpret_cast<const uint8_t*>(jsonMessage.constData()),
			jsonMessage.size());

		if (root["op"].error() == simdjson::SUCCESS)
		{
			qDebug() << "[Bybit WS System]"
				<< QString::fromUtf8(jsonMessage.constData(), jsonMessage.size());
			return std::nullopt;
		}
		if (root["topic"].error() == simdjson::SUCCESS)
		{
			std::string_view topic = root["topic"].get_string().value();

			if (topic.starts_with("kline"))
			{
				// Парсим топик: "kline.5.BTCUSDT"
				// parts[0]="kline"  parts[1]="5"  parts[2]="BTCUSDT"
				QString     topicStr = QString::fromUtf8(topic.data(), topic.size());
				QStringList parts = topicStr.split('.');

				if (parts.size() == 3)
				{
					outSymbol = parts[2];
					outInterval = intervalFromBybitString(parts[1]);
				} else
				{
					qWarning() << "[ByBitParser] Unexpected topic format:" << topicStr;
					return std::nullopt;
				}

				simdjson::dom::array data_array = root["data"].get_array().value();
				if (data_array.size() > 0)
				{
					simdjson::dom::object candleData = data_array.at(0).get_object().value();

					Candle liveCandle;
					liveCandle.timestamp = candleData["start"].get_int64().value() / 1000;
					liveCandle.open = fastParseDouble(candleData["open"].get_string().value());
					liveCandle.high = fastParseDouble(candleData["high"].get_string().value());
					liveCandle.low = fastParseDouble(candleData["low"].get_string().value());
					liveCandle.close = fastParseDouble(candleData["close"].get_string().value());
					liveCandle.volume = fastParseDouble(candleData["volume"].get_string().value());

					return liveCandle;
				}
			}
		}
	} catch (const simdjson::simdjson_error& er)
	{
		qDebug() << "[ByBitParser] Live candle parsing error:" << er.what();
	}
	return std::nullopt;
}

std::optional<OrderBookSnapshot> ByBitParser::parseOrderBook(const QByteArray& jsonMessage, QString& outSymbol, bool& outIsDelta)
{
	simdjson::dom::parser parser;
	try
	{
		simdjson::dom::element root = parser.parse(reinterpret_cast<const uint8_t*>(jsonMessage.constData()), jsonMessage.size());//из const char* to const uint8_t он заставляет чиать теже самые биты только по другим правилам!!!
		if (root["topic"].error() != simdjson::SUCCESS)return std::nullopt;

		std::string_view topic = root["topic"].get_string().value();
		if (!topic.starts_with("orderbook")) return std::nullopt;

		std::string_view type = root["type"].get_string().value();
		outIsDelta = (type == "delta");

		simdjson::dom::element data = root["data"];
		std::string_view sym = data["s"].get_string().value();
		outSymbol = QString::fromUtf8(sym.data(), sym.size());

		OrderBookSnapshot snap; // по идеи созавать каждый раз снэп может быть накладно может .clear и снэп перенести в private хотя хз
		snap.exchange = "Bybit";
		snap.symbol = outSymbol;

		for (simdjson::dom::element row : data["b"].get_array().value())
		{
			simdjson::dom::array pair = row.get_array().value();
			simdjson::dom::array::iterator it = pair.begin();
			OrderBookLevel lv;
			lv.price = fastParseDouble((*it).get_string().value());
			++it;
			lv.qty = fastParseDouble((*it).get_string().value());
			snap.bids.push_back(lv);
		}
		for (simdjson::dom::element row : data["a"].get_array().value())
		{
			simdjson::dom::array pair = row.get_array().value();
			simdjson::dom::array::iterator it = pair.begin();
			OrderBookLevel lv;
			lv.price = fastParseDouble((*it).get_string().value());
			++it;
			lv.qty = fastParseDouble((*it).get_string().value());
			snap.asks.push_back(lv);
		}

		std::sort(snap.asks.begin(), snap.asks.end(), [](const OrderBookLevel& a, const OrderBookLevel& b)
			{
				return a.price < b.price;
			});
		std::sort(snap.bids.begin(), snap.bids.end(), [](const OrderBookLevel& a, const OrderBookLevel& b)
			{
				return a.price > b.price;
			});
		return snap;

	}catch (const simdjson::simdjson_error& er)
	{
		qDebug() << "[ByBitParser] OrderBook parse error: " << er.what();
		return std::nullopt;
	}
}

std::vector<TradeTick> ByBitParser::parseTrades(const QByteArray& jsonMessage, QString& outSymbol)
{
	std::vector<TradeTick> result;
	simdjson::dom::parser parser;
	try
	{
		simdjson::dom::element root = parser.parse(
			reinterpret_cast<const uint8_t*>(jsonMessage.constData()), jsonMessage.size());

		if (root["topic"].error() != simdjson::SUCCESS) return result;

		std::string_view topic = root["topic"].get_string().value();
		if (!topic.starts_with("publicTrade")) return result;

		simdjson::dom::array data = root["data"].get_array().value();
		for (simdjson::dom::element item : data)
		{
			TradeTick tick;

			// Timestamp milliseconds → "HH:mm:ss"
			int64_t ts = static_cast<int64_t>(item["T"].get_int64().value());
			QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts, Qt::UTC);
			tick.time = dt.toString("HH:mm:ss");

			// Symbol
			std::string_view sym = item["s"].get_string().value();
			outSymbol = QString::fromUtf8(sym.data(), sym.size());

			// Side: "Buy" = true
			std::string_view side = item["S"].get_string().value();
			tick.isBuy = (side == "Buy");

			// Price and quantity are strings in Bybit publicTrade
			tick.price = fastParseDouble(item["p"].get_string().value());
			tick.qty   = fastParseDouble(item["v"].get_string().value());

			result.push_back(tick);
		}
	}
	catch (const simdjson::simdjson_error& er)
	{
		qDebug() << "[ByBitParser] parseTrades error:" << er.what();
	}
	return result;
}

//==WebSocket outgoing================================================================
QString ByBitParser::buildSubscriptionRequest(const QString& topic, const QString& reqId)
{
	return QString(R"({"req_id":"sub_%1","op":"subscribe","args":["%2"]})")
		.arg(reqId, topic);
}
QString ByBitParser::buildUnsubscribeRequest(const QString& topic, const QString& reqId)
{
	return QString(R"({"req_id":"unsub_%1","op":"unsubscribe","args":["%2"]})")
		.arg(reqId, topic);
}

QString ByBitParser::buildPingRequest()
{
	return QString(R"({"req_id":"ping_%1","op":"ping"})")
		.arg(QDateTime::currentMSecsSinceEpoch());
}
//==Конвертеры =========================================================================

ChartInterval ByBitParser::intervalFromBybitString(const QString& str)
{
	if (str == "D") return ChartInterval(ChartInterval::Unit::Day, 1);
	if (str == "W") return ChartInterval(ChartInterval::Unit::Week, 1);
	if (str == "M") return ChartInterval(ChartInterval::Unit::Month, 1);

	bool ok = false;
	int  value = str.toInt(&ok);

	if (!ok)
	{
		qWarning() << "[ByBitParser] Unknown interval string:" << str
			<< "— fallback to Minute_1";
		return ChartInterval(ChartInterval::Unit::Minute, 1);
	}

	if (value < 60)  return ChartInterval(ChartInterval::Unit::Minute, value);
	if (value % 60 == 0) return ChartInterval(ChartInterval::Unit::Hour, value / 60);

	qWarning() << "[ByBitParser] Cannot map interval:" << value
		<< "— fallback to Minute_1";
	return ChartInterval(ChartInterval::Unit::Minute, 1);
}
