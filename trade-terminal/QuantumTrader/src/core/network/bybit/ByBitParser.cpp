#include"ByBitParser.h"
#include<QJsonDocument>
#include<QJsonObject>
#include<QJsonArray>
#include<QJsonParseError>
#include<QDebug>
#include<QVariant>



int ByBitParser::parserPingResponse(const QByteArray& rawData, QString& outErrorMsg)
{
	QJsonParseError parserError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData, &parserError);

	if (jsonDoc.isNull() || parserError.error != QJsonParseError::NoError)
	{
		outErrorMsg = "JSON Parse Error: " + parserError.errorString();
		return -1;
	}
	if (jsonDoc.isObject())
	{
		QJsonObject root = jsonDoc.object();
		int retCode = root.value("retCode").toInt();
		if (retCode != 0)
		{
			outErrorMsg = root.value("retMsg").toString();
		}
		return retCode;
	}
	return -1;
}

std::vector<Candle> ByBitParser::parseHistory(const QByteArray& rawData)
{
	std::vector<Candle> chunk;
	QJsonDocument doc = QJsonDocument::fromJson(rawData);
	if (doc.isNull()) return chunk;

	QJsonObject root = doc.object();
	if (root.value("retCode").toInt() != 0) return chunk;

	QJsonArray list = root["result"].toObject()["list"].toArray();
	chunk.reserve(list.size());

	for (int i = list.size() - 1; i >= 0; --i)
	{
		QJsonArray c = list[i].toArray();
		if (c.size() < 6) continue;

		Candle candle;
		candle.timestamp = c[0].toString().toLongLong() / 1000;
		candle.open = c[1].toString().toDouble();
		candle.high = c[2].toString().toDouble();
		candle.low = c[3].toString().toDouble();
		candle.close = c[4].toString().toDouble();
		candle.volume = c[5].toString().toDouble();
		chunk.push_back(candle);
	}
	return chunk;
}
std::optional<Candle> ByBitParser::parseLiveCandle(const QString& jsonMessage, QString& outSymbol)
{
	QJsonDocument doc = QJsonDocument::fromJson(jsonMessage.toUtf8());
	QJsonObject root = doc.object();

	if (root.contains("op"))
	{
		qDebug() << "[Bybit WS System] " << jsonMessage;
		return std::nullopt;
	}
	if (root.contains("topic") && root.value("topic").toString().startsWith("kline"))
	{
		QJsonArray dataArr = root.value("data").toArray();
		if (!dataArr.isEmpty())
		{
			QJsonObject candleData = dataArr.first().toObject();
			outSymbol = root.value("topic").toString().split(".").last();

			Candle liveCandle;
			liveCandle.timestamp = candleData.value("start").toVariant().toLongLong() / 1000;
			liveCandle.open = candleData.value("open").toString().toDouble();
			liveCandle.high = candleData.value("high").toString().toDouble();
			liveCandle.low = candleData.value("low").toString().toDouble();
			liveCandle.close = candleData.value("close").toString().toDouble();
			liveCandle.volume = candleData.value("volume").toString().toDouble();

			return liveCandle;
		}
	}
	return std::nullopt;
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

QString ByBitParser::buildPinqRequest()
{
	QJsonObject pingReq;
	pingReq["req-id"] = "ping_" + QString::number(QDateTime::currentMSecsSinceEpoch());
	pingReq["op"] = "ping";

	return QJsonDocument(pingReq).toJson(QJsonDocument::Compact);
}
