#include<QtTest/QtTest>
#include"core/marketdata/detectors/DetectorMath.h"
namespace
{
	using Trades = std::vector<TradeTick>;
}

class DetectorMathTest : public QObject
{
	Q_OBJECT
private slots:
	void sumTradeQty_emptyList_isZero()
	{
		const Trades trades;
		const double result = DetectorMath::sumTradedQtyAtPriceByAggressor(trades, 100.0, DetectorMath::kPriceEpsilon, true);

		QCOMPARE(result, 0.0);
	}
	void sumTradeQty_matchingTrades_areSummed()
	{
		const Trades trades = 
		{
			{0, 100.0, 5.0, true},		
			{0, 100.0, 4.0, true}
		};
		const double result = DetectorMath::sumTradedQtyAtPriceByAggressor(trades, 100.0, DetectorMath::kPriceEpsilon, true);
		QCOMPARE(result, 9.0);
	}
	void sumTradeQty_otherSide_isIgnored()
	{
		const Trades trades =
		{
			{0, 100.0, 5.0, true},
			{0, 100.0, 5.0, false}
		};
		const double result = DetectorMath::sumTradedQtyAtPriceByAggressor(trades, 100.0, DetectorMath::kPriceEpsilon, true);
		QCOMPARE(result, 5.0);

	}
	void sumTradeQty_otherPrice_isIgnored()
	{
		const Trades trades = 
		{
			{0, 100.0, 5.0, true},
			{0, 99.99, 4.0, true}
		};
		const double result = DetectorMath::sumTradedQtyAtPriceByAggressor(trades, 100.0, DetectorMath::kPriceEpsilon, true);
		QCOMPARE(result, 5.0);
	}
	void sumTradeQty_mixedTrades_countsOnlyMatching()
	{
		const Trades trades =
		{
			{0, 100.0, 5.0, true},
			{0, 99.8,4.5, true},
			{0, 100.0, 3.0, false},
			{0, 99.8, 3.5, false},
			{0, 99.8, 2.0, true}
		};
		const double result = DetectorMath::sumTradedQtyAtPriceByAggressor(trades, 99.8, DetectorMath::kPriceEpsilon, true);
		QCOMPARE(result, 6.5);
	}
};
QTEST_APPLESS_MAIN(DetectorMathTest)
#include "DetectorMathTest.moc"