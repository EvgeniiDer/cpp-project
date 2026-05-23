#include "CandleHistoryManager.h"
#include <algorithm>
#include <QDebug>
#include"../events/EventBus.h"

CandleHistoryManager::CandleHistoryManager(IExchangeConnector* connector, QObject* parent /* = nullptr */) : QObject(parent), m_connector(connector)
{
	QObject::connect(m_connector, &IExchangeConnector::historyChunkLoaded, this, &CandleHistoryManager::onChunkLoaded);
}
void CandleHistoryManager::loadDeepHistory(const QString& symbol, ChartInterval interval, int targetLimit)
{
	if (m_currentSymbol == symbol && !m_accumulated.empty())
	{
		qDebug() << "[HistoryManager] Lazy load requested.Fetching older data...";
		qint64 oldestTimeMs = m_accumulated.front().timestamp * 1000;

		m_connector->fetchHistory(symbol, interval, 1000, oldestTimeMs - 1);
		return;
	}
	m_currentSymbol = symbol;
	m_currentInterval = interval;
	m_targetLimit = targetLimit;
	
	m_accumulated.clear();
	m_accumulated.reserve(targetLimit);

	qDebug() << "[HistoryManager] Start load: " << symbol << "limit" << targetLimit;

	m_connector->fetchHistory(symbol, interval, std::min(1000, targetLimit), 0);
}
void CandleHistoryManager::onChunkLoaded(const QString& symbol, const std::vector<Candle>& chunk)
{
	if (symbol != m_currentSymbol)
	{
		qDebug() << "[HistoryManager] Ignored zombie chunk for: " << symbol << "(Current is" << m_currentSymbol << ")";
		return;
	}
	if (chunk.empty())
	{
		if (!m_accumulated.empty())
		{
			emit historyReady(m_currentSymbol, m_accumulated);
			//EventBus
			emit EventBus::instance().deepHistoryReady("Bybit", m_currentSymbol, m_accumulated);
		}
		return;
	}
	m_accumulated.insert(m_accumulated.begin(), chunk.begin(), chunk.end());
	qDebug() << "[HistoryManager] Buffer size updated:" << m_accumulated.size();
	emit historyReady(m_currentSymbol, m_accumulated);
	//EventBus

	emit EventBus::instance().deepHistoryReady("Bybit", m_currentSymbol, m_accumulated);
}
