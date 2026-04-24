#pragma once

#include<QObject>
#include<QString>
#include<QHash>
#include<functional>
#include<vector>

#include"../network/common/IExchangeConnector.h"
#include"../network/common/NetworkTypes.h"
#include"../models/Candle.h"

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
signals:
	void candlesUpdated(const QString& exchangeName, const QString& symbol, const std::vector<Candle>& candles);
	void statusChanged(const QString& exchangeName, const QString& statusMsg);
private:
	QHash<QString, ConnectorFactory> m_factories;
	QHash<QString, IExchangeConnector*> m_activeConnectors;

	void setupConnectorSignals(const QString& exchangeName, IExchangeConnector* connector);

};
