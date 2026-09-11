#include"OrderBookHistoryManager.h"
#include<QDebug>
#include<algorithm>
#include<cmath>
#include"core/events/EventBus.h"
#include "detectors/IcebergDetector.h"
#include "detectors/SpoofDetector.h"

OrderBookHistoryManager::OrderBookHistoryManager(int depthLevels, qint64 retentionsMs,  QObject* parent)
	: QObject(parent), m_depthLevels(depthLevels), m_retentionMs(retentionsMs)
{
	m_detectors.push_back(std::make_unique<SpoofDetector>());
	m_detectors.push_back(std::make_unique<IcebergDetector>());

	QObject::connect(&EventBus::instance(), &EventBus::tradeReceived, this, &OrderBookHistoryManager::onTradeReceived, Qt::UniqueConnection);
}

void OrderBookHistoryManager::watch(const QString& exchange, const QString& symbol, double tickSize)
{
	m_watchedExchange = exchange;
	m_watchedSymbol = symbol;
	m_tickSize = tickSize;
	// Очищаем буфер накопленных сделок, чтобы не смешивать данные разных инструментов.
	// Это критично при переключении символов, иначе старые трейды могут быть ошибочно
	// учтены при анализе нового стакана.
	m_pendingTrades.clear();
	m_tickSizeWarned = false;

	for (std::unique_ptr<IOrderBookFeatureDetector>& detector : m_detectors)
	{
		detector->reset();
	}
}
void OrderBookHistoryManager::onTradeReceived(const QString& exchange, const QString& symbol, const TradeTick& tick)
{
	if (exchange != m_watchedExchange || symbol != m_watchedSymbol)
	{
		return;
	}
	m_pendingTrades.push_back(tick);
}
void OrderBookHistoryManager::clearHistory()
{
	m_historyRows.clear();
	m_hasPrev1 = false;
	m_hasPrev2 = false;
	m_pendingTrades.clear();
	for (auto& detector : m_detectors)
	{
		detector->reset();
	}
}
QVector<OrderBookFeatureRow> OrderBookHistoryManager::takeHistory()const
{
	QVector<OrderBookFeatureRow> result;
	result.reserve(static_cast<int>(m_historyRows.size()));
	for (const OrderBookFeatureRow& row : m_historyRows)
	{
		result.append(row);
	}
	return result;
}
void OrderBookHistoryManager::evicOldRows(const qint64 currentTimestamp)
{
	const qint64 cutoff = currentTimestamp - m_retentionMs;
	while (!m_historyRows.empty() && m_historyRows.front().timestamp < cutoff)
	{
		m_historyRows.pop_front();
	}
}
float OrderBookHistoryManager::calcDepthImbalance(const OrderBookSnapshot& book) const
{
	double bidSum = 0.0, askSum = 0.0;
	const int levels = m_depthLevels;
	for (int i = 0; i < levels; ++i)
	{
		if (i < static_cast<int>(book.bids.size()))
		{
			bidSum += book.bids[i].qty;
		}
		if (i < static_cast<int>(book.asks.size()))
		{
			askSum += book.asks[i].qty;
		}
	}
	return(bidSum + askSum > 0.0)
		? static_cast<float>((bidSum - askSum) / (bidSum + askSum))
		: 0.0f;
}
/**
 * Преобразует цену (например, ask) в целочисленный номер тика (ключ уровня).
 * Это нужно, чтобы избежать неточностей сравнения double и использовать целые ключи
 * в картах/мапах для хранения данных по каждому ценовому уровню.
 *
 * Пример: m_tickSize = 0.25, цена ask = 100.25 → 100.25 / 0.25 = 401.0 → ключ 401.
 * Цена 100.30 (не кратная тику) округлится до ближайшего тика и тоже даст 401,
 * так как уровень стакана определён с точностью до тика.
 *
 * Безопасность: m_tickSize проверен на > 0 в recordState до вызова этого метода.
 */

void OrderBookHistoryManager::recordState(qint64 currentTimestamp, const OrderBookSnapshot& currentBook)
{
	if (currentBook.bids.empty() || currentBook.asks.empty())
	{
		return;
	}
	if (m_tickSize <= 0.0)//Шаг цены должен быть больше 0 если он меньше значит он не установлен
	{
		// tickSize должен быть задан через watch() ДО начала записи истории.
		// Если он <= 0 — это ошибка вызывающего кода (забыли watch(), либо
		// передали в него некорректное значение), а не штатная ситуация,
		// которую можно тихо обойти запасным эпсилоном.
		static bool warned = false;
		if (!warned)
		{
			qWarning() << "[OrderBookHistoryManager::recordState]: tickSize не задан для"
				<< m_watchedExchange << m_watchedSymbol
				<< "— вызови watch() с корректным tickSize перед записью истории. "
				"Кадр пропущен, история НЕ пишется, пока tickSize некорректен.";
			warned = true;
		}
		return;
	}
	double bestBid = currentBook.bids[0].price;
	double bestAsk = currentBook.asks[0].price;
	double spread = bestAsk - bestBid;
	if (spread <= 0.0)
	{
		return;
	}

	float imbalance1 = calcDepthImbalance(currentBook);
	float imbalanceSma3 = imbalance1;

	if (m_hasPrev1)
	{
		const float imbPrev1 = calcDepthImbalance(m_prev1);
		const float imbPrev2 = m_hasPrev2 ? calcDepthImbalance(m_prev2) : imbPrev1;
		imbalanceSma3 = (imbalance1 + imbPrev1 + imbPrev2) / 3.0f;
	}
	float depthBids = 0.0f, depthAsk = 0.0f;
	for (int i = 0; i < m_depthLevels; ++i)
	{
		if (i < currentBook.bids.size())
		{
			depthBids += static_cast<float>(currentBook.bids[i].qty);
		}
		if (i < currentBook.asks.size())
		{
			depthAsk += static_cast<float>(currentBook.asks[i].qty);
		}
	}
	OrderBookFeatureRow row;
	row.timestamp = currentTimestamp;
	row.bestBid = bestBid;
	row.bestAsk = bestAsk;
	row.spread = spread;
	row.imbalanceL1 = imbalance1;
	row.imbalanceSma3 = imbalanceSma3;
	row.volumeDepthBids = depthBids;
	row.volumeDepthAsks = depthAsk;
	row.midPrice = (bestBid + bestAsk) / 2.0;
	row.isValid = true;

	const DetectionContext ctx{
		&m_prev1,
		m_hasPrev1,
		&currentBook,
		currentTimestamp,
		m_tickSize,
		&m_pendingTrades,
		m_depthLevels
	};
	for (auto& detector : m_detectors)
	{
		detector->detect(ctx, row);
	}

	m_historyRows.push_back(row);
	evicOldRows(currentTimestamp);

	m_prev2 = m_prev1;
	m_prev1 = currentBook;
	m_hasPrev2 = m_hasPrev1;
	m_hasPrev1 = true;

	m_pendingTrades.clear();
}






