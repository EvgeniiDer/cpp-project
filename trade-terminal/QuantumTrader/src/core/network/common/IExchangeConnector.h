#pragma once
#include"NetworkTypes.h"
#include<QObject>
#include<QString>
#include<vector>


class IExchangeConnector : public QObject
{
	Q_OBJECT
public:
	explicit IExchangeConnector(QObject* parent) : QObject(parent){}

	virtual void connect() = 0;
	virtual void disconnect() = 0;
	
	virtual void fetchHistory(const MarketContext& ctx) = 0;

	virtual void subscribeQuotes(const MarketContext& ctx) = 0;
signals:
	void historyChunkLoaded(int chartId, const QString& symbol, const std::vector<Candle>& candles);
	void errorOccured(const QString& message);
	void historyLoadFailed(int chartId, const QString& symbol, const QString& errorStr);
};