#include "CandleHistoryManager.h"
#include <algorithm>
#include <QDebug>
#include"../events/EventBus.h"

CandleHistoryManager::CandleHistoryManager(IExchangeConnector* connector,const QString& exchangeName, QObject* parent /* = nullptr */) : QObject(parent), m_connector(connector), m_exchangeName(exchangeName)
{
	QObject::connect(m_connector, &IExchangeConnector::historyChunkLoaded, this, &CandleHistoryManager::onChunkLoaded);
}
void CandleHistoryManager::loadDeepHistory(const MarketContext& ctx)
{
	if (m_currentSymbol == ctx.symbol && m_currentMarketType == ctx.marketType && m_currentInterval == ctx.interval && !m_accumulated.empty())
	{
		qDebug() << "[HistoryManager] Lazy load requested.Fetching older data...";
		qint64 oldestTimeMs = m_accumulated.front().timestamp * 1000;
		
		m_targetLimit += ctx.limit;

		MarketContext lazyCtx = ctx;
		lazyCtx.limit = 1000;
		lazyCtx.endTime = oldestTimeMs - 1;
		m_connector->fetchHistory(lazyCtx);
		return;
	}
	m_currentSymbol = ctx.symbol;
	m_currentMarketType = ctx.marketType;
	m_currentInterval = ctx.interval;
	m_targetLimit = ctx.limit;
	
	m_accumulated.clear();
	m_accumulated.reserve(m_targetLimit);

	qDebug() << "[HistoryManager] Start load: " << ctx.symbol << "limit" << ctx.limit;
	
	MarketContext firstCtx = ctx;
	firstCtx.limit = std::min(1000, ctx.limit);
	firstCtx.endTime = 0;
	m_connector->fetchHistory(firstCtx);
}
void CandleHistoryManager::onChunkLoaded(int chartId,const QString& symbol, const std::vector<Candle>& chunk)
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
			std::sort(m_accumulated.begin(), m_accumulated.end(), [](const Candle& a, const Candle& b)
				{
					return a.timestamp < b.timestamp;
				});
			emit historyReady(m_currentSymbol, m_accumulated);
			//EventBus
			emit EventBus::instance().deepHistoryReady(chartId, m_exchangeName, m_currentSymbol, m_accumulated);
		}else
		{
			qWarning() << "[HistoryManager] No history found at all for symbol:" << m_currentSymbol;
	     	emit historyReady(m_currentSymbol, m_accumulated);
	    	emit EventBus::instance().deepHistoryReady(chartId, m_exchangeName, m_currentSymbol, m_accumulated);
		}
		return;
	}
	m_accumulated.insert(m_accumulated.end(), chunk.begin(), chunk.end());
	emit historyChunkAppended(m_currentSymbol, m_accumulated.size(), m_targetLimit);

	if (static_cast<int>(m_accumulated.size()) >= m_targetLimit)
	{
		std::sort(m_accumulated.begin(), m_accumulated.end(), [](const Candle& a, const Candle& b)
			{
				return a.timestamp < b.timestamp;
			});
		if (static_cast<int>(m_accumulated.size()) > m_targetLimit)
		{
			int extraCount = m_accumulated.size() - m_targetLimit;
			m_accumulated.erase(m_accumulated.begin(), m_accumulated.begin() + extraCount);
		}
		emit historyReady(m_currentSymbol, m_accumulated);
		emit EventBus::instance().deepHistoryReady(chartId ,m_exchangeName, m_currentSymbol, m_accumulated);
		qDebug() << "[HistoryManager] Deep load complete for" << m_currentSymbol << ". Total size:" << m_accumulated.size();
		return;
	}
	qint64 oldestCandleTimeMs = chunk.front().timestamp * 1000;
	int remainingCandles = m_targetLimit - static_cast<int>(m_accumulated.size());
	MarketContext nextCtx;
	nextCtx.chartId = chartId;
	nextCtx.exchange = m_exchangeName;
	nextCtx.symbol = m_currentSymbol;
	nextCtx.marketType = m_currentMarketType;
	nextCtx.interval = m_currentInterval;
	nextCtx.limit = std::min(1000, remainingCandles);
	nextCtx.endTime = oldestCandleTimeMs - 1;

	int nextChunkSize = std::min(1000, remainingCandles);
	qDebug() << "[HistoryManager] Requesting next chunk of size:" << nextChunkSize << "before timestamp:" << oldestCandleTimeMs;
	m_connector->fetchHistory(nextCtx);
}
