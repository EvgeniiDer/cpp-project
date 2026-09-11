#include"SpoofDetector.h"
#include<algorithm>
#include"DetectorMath.h"

void SpoofDetector::detect(const DetectionContext& ctx, OrderBookFeatureRow& outRow)
{
	if (!ctx.hasPrevBook || ctx.prevBook == nullptr || ctx.currentBook == nullptr || ctx.pendingTrades == nullptr)
	{
		return;
	}

	const OrderBookSnapshot& prevBook = *ctx.prevBook;
	const OrderBookSnapshot& currentBook = *ctx.currentBook;

	if (currentBook.bids.empty() || currentBook.asks.empty())
	{
		return;
	}

	const double bestBid = currentBook.bids[0].price;
	const double bestAsk = currentBook.asks[0].price;

	if (!prevBook.bids.empty() && DetectorMath::priceEqual(prevBook.bids[0].price, bestBid))
	{
		// На сколько упал объём на этом уровне: было - стало
		if (double drop = prevBook.bids[0].qty - currentBook.bids[0].qty;//на сколько сильноу уменьшился обьем
			drop > 0.0)  // объём реально уменьшился
		{
			// Сколько из этого падения можно объяснить совершёнными сделками
			// (по цене bestBid или вблизи неё, с учётом шага цены)
			const double traded = DetectorMath::sumTradedQtyNearPrice(*ctx.pendingTrades, bestBid, ctx.tickTolerance); //какая была проторговка коллиичество сумма

			//unexplained — это та часть падения, которую нельзя списать на реальную торговлю. Если она положительная — есть признак снятия заявок без исполнения (спуф). Если 0 — всё чисто.
			const double unexplained = std::max(0.0, drop - traded);
			// === РАСЧЁТ СПУФ-ПРИЗНАКОВ ===
			// drop – насколько упал объём на этом ценовом уровне между снимками.
			// traded – сколько проторговалось по этой (или очень близкой) цене.
			// unexplained – объём, который пропал НЕ из-за сделок (потенциальная отмена / спуфинг).
			//
			// Если traded >= drop, то всё падение объяснено рынком – unexplained = 0.
			// Если traded < drop, разница это необъяснённое исчезновение – признак отмен.//
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

			outRow.bidSpoofDrop = static_cast<float>(unexplained);
			outRow.bidFakeRatio = static_cast<float>(unexplained / drop);
		}
	}
		//------Спуфинг на Аске ------
	if (!prevBook.asks.empty() && DetectorMath::priceEqual(prevBook.asks[0].price, bestAsk))
		{
		if (const double drop = prevBook.asks[0].qty - currentBook.asks[0].qty; drop > 0.0)
			{
				const double traded = DetectorMath::sumTradedQtyNearPrice(*ctx.pendingTrades, bestAsk, ctx.tickTolerance);
				const double unexplained = std::max(0.0, drop - traded);
				outRow.askSpoofDrop = static_cast<float>(unexplained);
				outRow.askFakeRatio = static_cast<float>(unexplained / drop);
			}
		}
}

QString SpoofDetector::name()const
{
	return QStringLiteral("SpoofDetector");
}
