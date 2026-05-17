#include "MarketDataManager.h"
#include<QDebug>
#include"../../core/storage/CandleHistoryManager.h"

MarketDataManager::MarketDataManager(QObject* parent /* = nullptr */)
	:QObject(parent)
{

}
MarketDataManager::~MarketDataManager()
{

}
void MarketDataManager::registerFactory(const QString& exchangeName, ConnectorFactory factory)
{
	if (!m_factories.contains(exchangeName))
	{
		m_factories.insert(exchangeName, factory);
		qDebug() << "[DataManager] Registered factory for: " << exchangeName;
	}
}
void MarketDataManager::setupConnectorSignals(const QString& exchangeName, IExchangeConnector* connector)
{
	connect(connector, &IExchangeConnector::stateChanged, this, [this, exchangeName](ConnectionState state)
		{
			QString msg;
			switch (state)
			{
			case ConnectionState::Connecting:
				msg = "Connecting...";
				break;
			case ConnectionState::Connected:
				msg = "Connected";
				break;
			case ConnectionState::Disconnected:
				msg = "Disconnected";
				break;
			case ConnectionState::Error:
				msg = "Error";
				break;
			}
			emit statusChanged(exchangeName, msg);
		});
	QObject::connect(connector, &IExchangeConnector::candlesReceived, this, [this, exchangeName](const QString& symbol, const std::vector<Candle>&candles)
		{
			qDebug() << "[DataManager] Routing" << candles.size() << "candles from" << exchangeName << "for" << symbol;
			emit candlesUpdated(exchangeName,symbol, candles);
		});
}
void MarketDataManager::connectTo(const QString& exchangeName)
{
	if (m_activeConnectors.contains(exchangeName))
	{
		qDebug() << "[DataManager] Already connected to" << exchangeName;
		return;
	}
	if (!m_factories.contains(exchangeName))
	{
		qDebug() << "[DataManager] CRITICAL ERROR: Factory for" << exchangeName << "not found";
		return;
	}
	emit statusChanged(exchangeName, "Connecting.....");
	IExchangeConnector* connector = m_factories[exchangeName](this);

	m_activeConnectors.insert(exchangeName, connector);
	setupConnectorSignals(exchangeName, connector);

	CandleHistoryManager* historyManager = new CandleHistoryManager(connector, this);
	m_historyManagers.insert(exchangeName, historyManager);

	QObject::connect(historyManager, &CandleHistoryManager::historyReady, this, [this, exchangeName](const QString& symbol, const std::vector<Candle>& candles)
		{
			qDebug() << "[DataManager] Deep history assembled! Routing to Chart:" << candles.size() << "candles.";
			emit candlesUpdated(exchangeName, symbol, candles);
		});
	connector->connect();
}
void MarketDataManager::requestHistory(const QString& exchangeName, const QString& symbol, ChartInterval interval, int limit)
{
	if (m_historyManagers.contains(exchangeName))
	{
		qDebug() << "[DataManager] Passing history request to CandleHistoryManager for:" << exchangeName;
		m_historyManagers[exchangeName]->loadDeepHistory(symbol, interval, limit);
	} else
	{
		qDebug() << "[DataManager] Cannot request history: HistoryManager for" << exchangeName << "is not active.";
	}

}

void MarketDataManager::subcribeToStream(const QString& exchangeName, const QString& symbol)
{
	if (m_activeConnectors.contains(exchangeName))
	{
		qDebug() << "[DataManager] Subscribing to WS stream for" << symbol << " on " << exchangeName;
		m_activeConnectors[exchangeName]->subscribeQuotes(symbol);
	}
	else
	{
		qDebug() << "[DataManager] Cannot subscribe: Connector" << exchangeName << "is not active.";
	}
}
