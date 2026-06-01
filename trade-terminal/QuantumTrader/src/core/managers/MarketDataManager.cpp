#include "MarketDataManager.h"
#include<QDebug>
#include"../../core/storage/CandleHistoryManager.h"
#include"../events/EventBus.h"
MarketDataManager::MarketDataManager(QObject* parent /* = nullptr */)
	:QObject(parent)
{
	QObject::connect(&EventBus::instance(), &EventBus::availableSymbolsLoaded, this, [this](const QList<std::pair<QString, QString>>& symbols)
		{
			if (!symbols.isEmpty())
			{
				m_cachedSymbols["Bybit"] = symbols;
				qDebug() << "[DataManager] Successfully intercepted and cached" << symbols.size() << "symbols for Bybit";
			}
		});
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
	IExchangeConnector* connector = m_factories[exchangeName](this);

	m_activeConnectors.insert(exchangeName, connector);

	CandleHistoryManager* historyManager = new CandleHistoryManager(connector, exchangeName, this);
	m_historyManagers.insert(exchangeName, historyManager);
	connector->connect();
}
void MarketDataManager::requestHistory(const MarketContext& ctx)
{
	if (m_historyManagers.contains(ctx.exchange))
	{
		qDebug() << "[DataManager] Passing history request to CandleHistoryManager for:" << ctx.exchange;
		m_historyManagers[ctx.exchange]->loadDeepHistory(ctx);
	} else
	{
		qDebug() << "[DataManager] Cannot request history: HistoryManager for" << ctx.exchange << "is not active.";
	}

}

void MarketDataManager::subcribeToStream(const QString& exchangeName, const QString& symbol, const QString& marketType)
{
	if (m_activeConnectors.contains(exchangeName))
	{
		qDebug() << "[DataManager] Subscribing to WS stream for" << symbol << " on " << exchangeName;
		m_activeConnectors[exchangeName]->subscribeQuotes(symbol, marketType);
	}
	else
	{
		qDebug() << "[DataManager] Cannot subscribe: Connector" << exchangeName << "is not active.";
	}
}

QList<std::pair<QString, QString>> MarketDataManager::getCachedSymbols(const QString& exchangeName) const
{
	return m_cachedSymbols.value(exchangeName);
}
