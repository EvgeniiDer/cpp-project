#pragma once

#include<QObject>
#include<QString>
#include<QHash>
#include<functional>
#include<vector>

#include"../network/common/IExchangeConnector.h"
#include"../network/common/NetworkTypes.h"
#include"../models/Candle.h"

class CandleHistoryManager;
class MarketDataManager : public QObject
{
	Q_OBJECT
public:
	using ConnectorFactory = std::function <IExchangeConnector* (QObject* parent)>;
	explicit MarketDataManager(QObject* parent = nullptr);
	~MarketDataManager();

	void registerFactory(const QString& exchangeName, ConnectorFactory factory);
	void connectTo(const QString& exchangeName);
	void requestHistory(const QString& exchangeName, const QString& symbol, ChartInterval interval, int limit);
	void subcribeToStream(const QString& exchangeName, const QString& symbol);
signals:
	void candlesUpdated(const QString& exchangeName, const QString& symbol, const std::vector<Candle>& candles);
	void statusChanged(const QString& exchangeName, const QString& statusMsg);
	//TODO WHATCHLIST next!!!
	void tickerUpdated(const QString& exchangeName, const QString& symbol, double lastPrice, double volume);
private:
	QHash<QString, ConnectorFactory> m_factories;
	QHash<QString, IExchangeConnector*> m_activeConnectors;

	QHash<QString, CandleHistoryManager*>m_historyManagers;
	void setupConnectorSignals(const QString& exchangeName, IExchangeConnector* connector);

};
