#include"IcebergDetector.h"

#include <qcontainerinfo.h>

#include"DetectorMath.h"
#include "simdjson.h"

QString IcebergDetector::name() const
{
	return QStringLiteral("IcebergDetector");
}

void IcebergDetector::reset()
{
	m_bidLevels.clear();
	m_askLevels.clear();
	m_bidWalkCandidates.clear();
	m_askWalkCandidates.clear();
}

qint64 IcebergDetector::priceToTickKey(const double price, const double tickSize) 
{
	//В биржевых и торговых системах цены дискретны(изменяются на минимальный шаг  тик).Хранить цену как double в
	//ключах контейнеров(например, QMap или std::unordered_map) неудобно из - за погрешностей.Преобразование в
	//целочисленный тик - индекс позволяет :
	//Использовать цену как индекс в массиве или ключ в хеш - таблице без потери точности.
	//	Быстро сравнивать цены(сравнение целых чисел быстрее).
	//	Экономить память.

	//	Пример:
	//  Тик = 0.01 (1 цент).
	//	Цена = 100.50 → 100.50 / 0.01 = 10050 → ключ = 10050.
	//	Цена = 100.51 → 100.51 / 0.01 = 10051 → ключ = 10051.

	//Таким образом, по ключу можно восстановить цену : price = key * tickSize.



	return static_cast<qint64>(std::llround(price / tickSize));
}

void IcebergDetector::evictStaleLevels(const qint64 currentTimestamp)
{
	const qint64 cutoff = currentTimestamp - kLevelRetentionMs;

	auto evictFrom = [cutoff](std::map<qint64, IcebergLevelState>& state)
		{
			for (std::map<qint64, IcebergLevelState>::iterator it = state.begin(); it != state.end(); )
			{
				if (it->second.lastSeenTimestamp < cutoff)
				{
					it = state.erase(it);
				} else
					++it;
			}
		};
	evictFrom(m_bidLevels);
	evictFrom(m_askLevels);
}

IcebergDetector::IcebergSideResult IcebergDetector::processSide(const std::vector<OrderBookLevel>& bookLevels, 
																IcebergSideState sideState, 
																const DetectionContext& ctx)
{
	IcebergSideResult result;

	// Чистим устаревших кандидатов на "переставленный" айсберг - если
	// дозаправка не случилась за kIcebergWalkWindowMs, айсберг, скорее
	// всего, просто закончился и больше не появится.
	std::deque<IcebergWalkCandidate>& walkCandidates = *sideState.walkCandidates;
	while (!walkCandidates.empty() && ctx.currentTimestamp - walkCandidates.front().createdTimestamp > kIcebergWalkWindowMs)
	{
		walkCandidates.pop_front();
	}

	std::map<qint64, IcebergLevelState>& state = *sideState.levelStates;
	const int limit = std::min(ctx.depthLevels, static_cast<int>(bookLevels.size()));

	for (int i = 0; i < limit; ++i)
	{
		const double price = bookLevels[i].price;
		const double qty = bookLevels[i].qty;
		const qint64 key = priceToTickKey(price, ctx.tickTolerance);

		IcebergLevelState& entry = state[key]; // создаст новую запись, если уровня ещё не было

		if (!entry.initialized)
		{
			// Первая встреча этого ценового уровня - не с чем сравнивать.
			entry.lastQty = qty;
			entry.lastSeenTimestamp = ctx.currentTimestamp;
			entry.initialized = true;
			continue;
		}

		const double drop = entry.lastQty - qty;

		if (drop > 0.0)
		{
			// Объём уменьшился - проверяем, реальная ли это проторговка.
			const double traded = DetectorMath::sumTradedQtyNearPrice(*ctx.pendingTrades, price, ctx.tickTolerance);
			const double explainedRatio = traded / drop;
			// если проторговка была 28 а drop  30 28/30 = 0.93 это небольшая погрешность и можно сказать что у нас было реальная
			// проторговка которая обьясняет снижение обьема в стакане на данном уровне мы дальше сравниваем с 	static constexpr double kExplainedRatioThreshold = 0.9(тоесть тут мы говорим о погрешности небльшой)

			if (explainedRatio >= kExplainedRatioThreshold)
			{
				// Похоже на реальное исполнение (не спуф) - берём уровень
				// "на карандаш": если объём сюда вернётся - это дозаправка.
				entry.awaitingRefill = true;
				walkCandidates.push_back({ key, ctx.currentTimestamp, traded });
			} else
			{
				// Объём пропал необъяснимо - это епархия SpoofDetector,
				// сбрасываем ожидание, чтобы не путать два паттерна.
				entry.awaitingRefill = false;
			}
		} else if (drop < 0.0)
		{
			const double refill = -drop;

			if (entry.awaitingRefill)
			{
				// Дозаправка на ТОЙ ЖЕ цене.
				result.refillCount += 1;
				result.refillQty += static_cast<float>(refill);
				entry.awaitingRefill = false;
			} else
			{
				// Дозаправки тут не ждали - может, айсберг "переставился"
				// на соседний тик? Ищем ближайшего кандидата в очереди.
				auto it = std::find_if(walkCandidates.begin(), walkCandidates.end(),[&](const IcebergWalkCandidate& p)
					{
						return std::llabs(p.priceTickKey - key) <= kIcebergWalkTicks;
					});
				if (it != walkCandidates.end())
				{
					result.refillCount += 1;
					result.refillQty += static_cast<float>(refill);
					walkCandidates.erase(it);
				}
			}
		} else // drop == 0.0
		{
			const double traded = DetectorMath::sumTradedQtyNearPrice(*ctx.pendingTrades, price, ctx.tickTolerance);
			if (traded > 0.0)
			{
				result.instantRefillCount += 1;
				result.instantRefillQty += static_cast<float>(traded);
			}
		}

		entry.lastQty = qty;
		entry.lastSeenTimestamp = ctx.currentTimestamp;
	}

	return result;
}

void IcebergDetector::detect(const DetectionContext& ctx, OrderBookFeatureRow& outRow)
{
	if (ctx.currentBook == nullptr || ctx.pendingTrades == nullptr)
	{
		return;
	}

	evictStaleLevels(ctx.currentTimestamp);

	IcebergSideState bidState{ &m_bidLevels, &m_bidWalkCandidates };
	IcebergSideState askState{ &m_askLevels, &m_askWalkCandidates };

	IcebergSideResult bidResult = processSide(ctx.currentBook->bids, bidState, ctx);
	IcebergSideResult askResult = processSide(ctx.currentBook->asks, askState, ctx);

	outRow.bidIcebergRefillCount = bidResult.refillCount;
	outRow.bidIcebergRefillQty = bidResult.refillQty;
	outRow.askIcebergRefillCount = askResult.refillCount;
	outRow.askIcebergRefillQty = askResult.refillQty;

	outRow.bidIcebergInstantRefillCount = bidResult.instantRefillCount;
	outRow.bidIcebergInstantRefillQty = bidResult.instantRefillQty;
	outRow.askIcebergInstantRefillCount = askResult.instantRefillCount;
	outRow.askIcebergInstantRefillQty = askResult.instantRefillQty;
}











