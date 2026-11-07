#include"AggressiveIcebergDetector.h"

#include<algorithm>
#include<cmath>
#include"DetectorMath.h"

QString AggressiveIcebergDetector::name() const
{
	return QStringLiteral("AggressiveIcebergDetector");
}
double AggressiveIcebergDetector::zoneTolernce(const DetectionContext& ctx, int ticks)
{
	return ctx.tickTolerance * (ticks + 0.5);
}
AggressiveIcebergDetector::RepeatCounts AggressiveIcebergDetector::countRepeatsAndRecord(const DetectionContext& ctx, const QString& side, double anchorPrice, float sweptQty)
{
	RepeatCounts counts;
	counts.wide = ctx.eventLog->countNear(MarketEventType::Sweep, side, anchorPrice, zoneTolernce(ctx, kRepeatZoneTicks));
	counts.narrow = ctx.eventLog->countNear(MarketEventType::Sweep, side, anchorPrice, zoneTolernce(ctx, kNarrowRepeatZoneTicks));

	ctx.eventLog->record(MarketEvent{ ctx.currentTimestamp, side, anchorPrice, MarketEventType::Sweep, sweptQty });
	return counts;
}


double AggressiveIcebergDetector::findQtyAtPrice(const std::vector<OrderBookLevel>& levels, double price, double priceEpsilon )
{
	for (const OrderBookLevel& level : levels)
	{
		if (std::fabs(level.price - price) <= priceEpsilon)
		{
			return level.qty;
		}
	}
	return 0.0;
}
AggressiveIcebergDetector::SweepSideResult AggressiveIcebergDetector::processSide(const std::vector<OrderBookLevel>& prevLevels, const std::vector<OrderBookLevel>& currentLevels, const DetectionContext& ctx, bool aggressorIsBuy) const
{
	SweepSideResult result;
	const int limit = std::min(kSweepScanDepth, static_cast<int>(prevLevels.size()));

	int consecutive = 0;//счетчик зачищенных уровней
	float sweptQty = 0.0f;// пропылесосенный обьем на всех уровнях

	for (int i = 0; i < limit; ++i)
	{
		const double prevPrice = prevLevels[i].price;
		const double prevQty = prevLevels[i].qty;

		const double currentQty = findQtyAtPrice(currentLevels, prevPrice, DetectorMath::kPriceEpsilon);
		const double drop = prevQty - currentQty;
		if (drop <= 0.0)
		{
			break;
		}
		const double traded = DetectorMath::sumTradedQtyAtPriceByAggressor(*ctx.pendingTrades, prevPrice, DetectorMath::kPriceEpsilon, aggressorIsBuy);
		const double explainRatio = traded / drop;

		const bool explainByTrades = explainRatio >= kExplainedRatioThreshold;
		//	Сценарий A — настоящий sweep :

		//  prevQty = 10, currentQty = 0  (уровень исчез из стакана целиком)
		//	drop = 10, fullyDepleted = (0 <= kPriceEpsilon) = TRUE

		//	Сценарий B — просто частичная проторговка(НЕ sweep) :

		//	prevQty = 10, currentQty = 7  (объём уменьшился, но уровень остался в книге)
		//	drop = 3, fullyDepleted = (7 <= kPriceEpsilon) = FALSE
		const bool fullyDepleted = currentQty <= DetectorMath::kPriceEpsilon;

		if (!explainByTrades || !fullyDepleted)
		{
			break;
		}
		++consecutive;
		sweptQty += static_cast<float>(traded);
		//if (!currentLevels.empty())
		//{
		//	const double newTopPrice = currentLevels[0].price;
		//	const double newTopQty = currentLevels[0].qty;
		//	const bool matchesNextUntouchedLevel = consecutive < static_cast<int>(prevLevels.size())&& std::fabs(prevLevels[consecutive].price - newTopPrice) <= ctx.tickTolerance;
		//	if (!matchesNextUntouchedLevel)
		//	{
		//		result.restedQty = static_cast<float>(newTopQty);
		//	}
		//}
	}
	result.sweptLevels = consecutive;
	result.sweptQty = sweptQty;
	return result;
}
void AggressiveIcebergDetector::detect(const DetectionContext& ctx, OrderBookFeatureRow& outRow)
{
	if (!ctx.hasPrevBook || ctx.prevBook == nullptr || ctx.currentBook == nullptr || ctx.pendingTrades == nullptr)
	{
		return;
	}

	const OrderBookSnapshot& prevBook = *ctx.prevBook;
	const OrderBookSnapshot& currentBook = *ctx.currentBook;

	if (prevBook.bids.empty() || prevBook.asks.empty())
	{
		return;
	}

	const SweepSideResult bidResult = processSide(prevBook.bids, currentBook.bids, ctx, false);
	const SweepSideResult askResult = processSide(prevBook.asks, currentBook.asks, ctx, true);

	outRow.bidSweepLevels = bidResult.sweptLevels;
	outRow.bidSweepQty = bidResult.sweptQty;
	outRow.bidSweepRestedQty = bidResult.restedQty;

	outRow.askSweepLevels = askResult.sweptLevels;
	outRow.askSweepQty = askResult.sweptQty;
	outRow.askSweepRestedQty = askResult.restedQty;

	if (ctx.eventLog == nullptr)
	{
		return;
	}

	ctx.eventLog->evictStale(ctx.currentTimestamp);

	if (bidResult.sweptLevels >= kMinSweepLevels)
	{
		const RepeatCounts repeats = countRepeatsAndRecord(ctx, QStringLiteral("bid"), prevBook.bids[0].price, bidResult.sweptQty);
		outRow.bidSweepRepeatCount = repeats.wide;
		outRow.bidSweepRepeatCountNarrow = repeats.narrow;
	}

	if (askResult.sweptLevels >= kMinSweepLevels)
	{
		const RepeatCounts repeats = countRepeatsAndRecord(ctx, QStringLiteral("ask"), prevBook.asks[0].price, askResult.sweptQty);
		outRow.askSweepRepeatCount = repeats.wide;
		outRow.askSweepRepeatCountNarrow = repeats.narrow;
	}
}



