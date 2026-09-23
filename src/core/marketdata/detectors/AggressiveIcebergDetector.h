#pragma once
#include"IOrderBookFeatureDetector.h"

// Детектор агрессивного "пылесоса" (sweep): в отличие от IcebergDetector
// (тихая дозаправка на месте/соседнем тике, без пересечения спреда) -
// детектирует, когда сразу НЕСКОЛЬКО верхних уровней стакана вычищаются
// за один кадр (currentBook vs prevBook) подряд, объяснённым сделками
// объёмом. Это "проедание" книги заявок вглубь, а не пошаговое
// стаптывание одного уровня.
//
// Сам по себе класс без состояния между тиками - сравнивает только два
// соседних снепшота, как SpoofDetector. Историю повторяющихся sweep-
// событий (поиск "интереса" к одной и той же цене со временем) не хранит
// сам - для этого используется общий ctx.eventLog (MarketEventLog).
class AggressiveIcebergDetector : public IOrderBookFeatureDetector
{
public:
	void detect(const DetectionContext& ctx, OrderBookFeatureRow& outRow) override;
	[[nodiscard]] QString name() const override;

private:
	struct SweepSideResult
	{
		int sweptLevels = 0;    // сколько уровней подряд подтверждённо вычищено
		float sweptQty = 0.0f;  // суммарный объём, снятый с этих уровней
		float restedQty = 0.0f; // остаток агрессора, осевший новой заявкой на границе (0 - если полностью "влился в рынок")
	};

	SweepSideResult processSide(const std::vector<OrderBookLevel>& prevLevels,
		const std::vector<OrderBookLevel>& currentLevels,
		const DetectionContext& ctx) const;

	[[nodiscard]] static double findQtyAtPrice(const std::vector<OrderBookLevel>& levels, double price, double tolerance);

	static constexpr int kMinSweepLevels = 2;
	static constexpr double kExplainedRatioThreshold = 0.9;
};