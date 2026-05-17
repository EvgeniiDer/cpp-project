#include "CandleHistoryManager.h"
#include <algorithm>
#include <QDebug>


CandleHistoryManager::CandleHistoryManager(IExchangeConnector* connector, QObject* parent /* = nullptr */) : QObject(parent), m_connector(connector)
{
	QObject::connect(m_connector, &IExchangeConnector::historyChunkLoaded, this, &CandleHistoryManager::onChunkLoaded);
}
void CandleHistoryManager::loadDeepHistory(const QString& symbol, ChartInterval interval, int targetLimit)
{
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
		}
		return;
	}

	qint64 oldestTimeMs = chunk.front().timestamp * 1000;
	m_accumulated.insert(m_accumulated.begin(), chunk.begin(), chunk.end());
	qDebug() << "[HistoryManager] Progress: " << m_accumulated.size() << "/" << m_targetLimit;
	
	if (m_accumulated.size() >= static_cast<size_t>(m_targetLimit) || chunk.size() < 1000)
	{
		qDebug() << "[HistoryManager] Done! Total:" << m_accumulated.size();
		emit historyReady(m_currentSymbol, m_accumulated);
	} else
	{
		int nextLimit = std::min(1000, m_targetLimit - static_cast<int>(m_accumulated.size()));//TODO Приведение типа!!!!!!ИЗМЕНИТЬ
		m_connector->fetchHistory(m_currentSymbol, m_currentInterval, nextLimit, oldestTimeMs - 1);
	}
}
