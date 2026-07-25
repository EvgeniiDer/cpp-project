#include"OrderBookHistoryManager.h"
#include<QDebug>
#include<algorithm>
#include<cmath>
#include"core/events/EventBus.h"

namespace
{
	constexpr double kPriceEpsilon = 1e-9;
	bool priceEqual(double a, double b)
	{
		return std::fabs(a - b) < kPriceEpsilon;
	}
	double sumTradedQtyNearPrice(const std::vector<TradeTick>& trades, double price, double tickTolerance)
	{
		double sum = 0.0;
		for (const TradeTick& t: trades)
		{
			if (std::fabs(t.price - price) <= tickTolerance)
			{
				sum += t.qty;
			}
		}
		return sum;
	}
}
OrderBookHistoryManager::OrderBookHistoryManager(int depthLevels, qint64 retentionsMs,  QObject* parent)
	: QObject(parent), m_detpthLevels(depthLevels), m_retentionMs(retentionsMs)
{

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
void OrderBookHistoryManager::evicOldRows(qint64 currentTimestamp)
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
	const int levels = m_detpthLevels;
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

void OrderBookHistoryManager::recordState(qint64 currentTimestamp,const OrderBookSnapshot& currentBook)
{
	if (currentBook.bids.empty() || currentBook.asks.empty())
	{
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

	float bidSpoofDrop = 0.0f, askSpoofDrop = 0.0f;
	float bidFakeRatio = 0.0f, askFakeRatio = 0.0f;

	const double tickTolerance = (m_tickSize > 0.0)
		? m_tickSize
		: (kPriceEpsilon * 100.0);//шаг цены

	if (m_hasPrev1)
	{
		// ----- СПУФИНГ НА БИДЕ -----
		// Проверяем, что лучшая цена покупки НЕ изменилась (тот же самый уровень)
		if (!m_prev1.bids.empty() && priceEqual(m_prev1.bids[0].price, bestBid))
		{
			// На сколько упал объём на этом уровне: было - стало
			double drop = m_prev1.bids[0].qty - currentBook.bids[0].qty;//на сколько сильноу уменьшился обьем
			if (drop > 0.0)  // объём реально уменьшился
			{
				// Сколько из этого падения можно объяснить совершёнными сделками
				// (по цене bestBid или вблизи неё, с учётом шага цены)
				double traded = sumTradedQtyNearPrice(m_pendingTrades, bestBid, tickTolerance); //какая была проторговка коллиичество сумма

				//unexplained — это та часть падения, которую нельзя списать на реальную торговлю. Если она положительная — есть признак снятия заявок без исполнения (спуф). Если 0 — всё чисто.
				double unexplained = std::max(0.0, drop - traded);

				// === РАСЧЁТ СПУФ-ПРИЗНАКОВ ===
				// drop – насколько упал объём на этом ценовом уровне между снимками.
				// traded – сколько проторговалось по этой (или очень близкой) цене.
				// unexplained – объём, который пропал НЕ из-за сделок (потенциальная отмена / спуфинг).
				//
				// Если traded >= drop, то всё падение объяснено рынком – unexplained = 0.
				// Если traded < drop, разница это необъяснённое исчезновение – признак отмен.
				//
				// bidFakeRatio = unexplained / drop  – доля "фейкового" падения.
				// 0.0 → всё чисто, 1.0 → 100% объёма было просто снято (спуфинг).
				//
				// Примеры:
				// drop | traded | unexplained (max(0, drop-traded)) | bidFakeRatio | Интерпретация
				// 100  |   0    |              100                |     1.0      | Весь объём – чистая отмена, спуфинг
				// 100  |  30    |               70                |     0.7      | 70% снято, 30% – реальные сделки
				// 100  |  90    |               10                |     0.1      | Только 10% подозрительно
				// 100  | 100    |                0                |     0.0      | Всё "съедено" сделками, спуфинга нет
				// 100  | 130    |                0 (max(0,-30))   |     0.0      | Сделок больше, чем падение – чисто

				bidSpoofDrop = static_cast<float>(unexplained);
				bidFakeRatio = static_cast<float>(unexplained / drop);
			}
		}
		//Комменты смотреть выше 
		if (!m_prev1.asks.empty() && priceEqual(m_prev1.asks[0].price, bestAsk))
		{
			double drop = m_prev1.asks[0].qty - currentBook.asks[0].qty;
			if (drop > 0.0)
			{
				double traded = sumTradedQtyNearPrice(m_pendingTrades, bestAsk, tickTolerance);
				double unexplained = std::max(0.0, drop - traded);
				askSpoofDrop = static_cast<float>(unexplained);
				askFakeRatio = static_cast<float>(unexplained / drop);
			}
		}
		float imbPrev1 = calcDepthImbalance(m_prev1);
		float imbPrev2 = m_hasPrev2 ? calcDepthImbalance(m_prev2) : imbPrev1;
		imbalanceSma3 = (imbalance1 + imbPrev1 + imbPrev2) / 3.0f;
	}
	float depthBids = 0.0f, depthAsk = 0.0f;
	for (int i = 0; i < m_detpthLevels; ++i)
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
	row.bidSpoofDrop = bidSpoofDrop;
	row.askSpoofDrop = askSpoofDrop;
	row.bidFakeRatio = bidFakeRatio;
	row.askFakeRatio = askFakeRatio;
	row.volumeDepthBids = depthBids;
	row.volumeDepthAsks = depthAsk;
	row.midPrice = (bestBid + bestAsk) / 2.0;
	row.isValid = true;

	m_historyRows.push_back(row);
	evicOldRows(currentTimestamp);

	m_prev2 = m_prev1;
	m_prev1 = currentBook;
	m_hasPrev2 = m_hasPrev1;
	m_hasPrev1 = true;

	m_pendingTrades.clear();
}





