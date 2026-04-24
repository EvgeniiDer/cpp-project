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
	virtual ~IExchangeConnector() = default;

	virtual void connect() = 0;
	virtual void disconnect() = 0;
	
	virtual void fetchHistory(const QString& symbol, ChartInterval interval, int limit = 200) = 0;

	virtual void subscribeQuotes(const QString& symbol) = 0;
signals:
	void stateChanged(ConnectionState newState);
	void candlesReceived(const QString& symbol, const std::vector<Candle>& candles);
	void errorOccured(const QString& message);
};