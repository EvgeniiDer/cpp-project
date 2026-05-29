#include"ByBitParser.h"
#include"../../../external/simdjson/simdjson.h"
#include<QDebug>
#include<QDateTime>
#include<charconv>
#include<algorithm>
#include<string_view>
#include<QJsonDocument>
#include<QStringList>
#include<QJsonArray>
#include<QJsonObject>

namespace
{
	double fastParseDouble(std::string_view sv)
	{
		double value = 0.0;
		std::from_chars_result res = std::from_chars(sv.data(), sv.data() + sv.size(), value);
		const char* ptr = res.ptr;
		std::errc ec = res.ec;
		if (ec != std::errc())
		{
			return 0.0;
		}
		return value;
	}

	int64_t fastParseInt64(std::string_view sv)
	{
		int64_t value = 0;
		std::from_chars_result res = std::from_chars(sv.data(), sv.data() + sv.size(), value);
		const char* ptr = res.ptr;
		std::errc ec = res.ec;
		if (ec != std::errc())
		{
			return 0;
		}
		return value;
	}
}
int ByBitParser::parserPingResponse(const QByteArray& rawData, QString& outErrorMsg)
{
	simdjson::dom::parser parser;
	try
	{
		simdjson::dom::element root = parser.parse(reinterpret_cast<const uint8_t*>(rawData.constData()), rawData.size());

		int64_t retCode = root["retCode"].get_int64().value();
		if (retCode != 0)
		{
			std::string_view retMsg = root["retMsg"].get_string().value();
			outErrorMsg = QString::fromUtf8(retMsg.data(), retMsg.size());
		}
		return static_cast<int>(retCode);
	} catch (const simdjson::simdjson_error& er)
	{
		outErrorMsg = "simdjson Error: " + QString::fromStdString(er.what());
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
		if (retCode != 0) return chunk;

		simdjson::ondemand::array list = doc["result"]["list"].get_array();
		chunk.reserve(1000);

		for (simdjson::ondemand::value row : list)
		{
			simdjson::ondemand::array candle_array = row.get_array();
			simdjson::ondemand::array_iterator it = candle_array.begin();

			Candle candle;
			candle.timestamp = fastParseInt64((*it).get_string().value()) / 1000; ++it;
			candle.open = fastParseDouble((*it).get_string().value()); ++it;
			candle.high = fastParseDouble((*it).get_string().value()); ++it;
			candle.low = fastParseDouble((*it).get_string().value()); ++it;
			candle.close = fastParseDouble((*it).get_string().value());    ++it;
			candle.volume = fastParseDouble((*it).get_string().value());

			chunk.push_back(candle);
		}
	}
	catch (const simdjson::simdjson_error& er)
	{
		qDebug() << "[simdjson] Critical error parsing history: " << er.what();
		return chunk;
	}

	std::reverse(chunk.begin(), chunk.end());
	return chunk;
}
std::optional<Candle> ByBitParser::parseLiveCandle(const QByteArray& jsonMessage, QString& outSymbol)
{
	simdjson::dom::parser parser;
	try
	{
		simdjson::dom::element root = parser.parse(reinterpret_cast<const uint8_t*>(jsonMessage.constData()), jsonMessage.size());

		if (root["op"].error() == simdjson::SUCCESS)
		{
			qDebug() << "[Bybit WS System] " << QString::fromUtf8(jsonMessage.constData(), jsonMessage.size());
			return std::nullopt;
		}

		if (root["topic"].error() == simdjson::SUCCESS)
		{
			std::string_view topic = root["topic"].get_string().value();
			if (topic.starts_with("kline"))
			{
				size_t last_dot = topic.find_last_of('.');
				if (last_dot != std::string_view::npos)
				{
					std::string_view sym_view = topic.substr(last_dot + 1);
					outSymbol = QString::fromUtf8(sym_view.data(), sym_view.size());
				}
				auto data_array = root["data"].get_array().value();
				if (data_array.size() > 0)
				{
					auto candleData = data_array.at(0).get_object().value();

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
		qDebug() << "[simdjson] Live candle parsing error: " << er.what();
	}
	return std::nullopt;
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

		auto list = root["result"]["list"].get_array().value();

		symbols.reserve(static_cast<int>(list.size()));

		for (simdjson::dom::element item : list)
		{
			std::string_view sym_view = item["symbol"].get_string().value();
			symbols.append(QString::fromUtf8(sym_view.data(), sym_view.size()));
		}
		symbols.sort();
	} catch (const simdjson::simdjson_error& er)
	{
		qDebug() << "[simdjson] Error parsing available symbols:" << er.what();
	}
	return symbols;
}
QString ByBitParser::buildSubscriptionRequest(const QString& symbol, const QString& interval)
{
	QString topic = QString("kline.%1.%2").arg(interval).arg(symbol);

	QJsonObject req;
	req["req_id"] = "sub_" + symbol;
	req["op"] = "subscribe";
	req["args"] = QJsonArray{ topic };
	return QJsonDocument(req).toJson(QJsonDocument::Compact);
}

QString ByBitParser::buildPingRequest()
{
	QJsonObject pingReq;
	pingReq["req-id"] = "ping_" + QString::number(QDateTime::currentMSecsSinceEpoch());
	pingReq["op"] = "ping";

	return QJsonDocument(pingReq).toJson(QJsonDocument::Compact);
}
