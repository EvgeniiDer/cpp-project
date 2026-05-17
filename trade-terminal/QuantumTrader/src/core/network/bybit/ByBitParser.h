#pragma once
#include<optional>
#include<vector>
#include<QString>
#include<QByteArray>
#include"../../models/Candle.h"
class ByBitParser
{
public:
	static int parserPingResponse(const QByteArray& rawData, QString& outErrorMsg);
	static std::vector<Candle>parseHistory(const QByteArray& rawData);
	static std::optional<Candle> parseLiveCandle(const QString& jsonMessage, QString& outSymbol);
	static QString buildSubscriptionRequest(const QString& symbol, const QString& interval);
	static QString buildPinqRequest();
};