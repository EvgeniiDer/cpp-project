#include <QtTest>
#include"core/marketdata/detectors/AggressiveIcebergDetector.h"

namespace
{
    constexpr double kTick = 0.1;
    using Levels = std::vector<OrderBookLevel>;
    using Trades = std::vector<TradeTick>;

    OrderBookSnapshot book(Levels bids, Levels asks)
    {
        OrderBookSnapshot b;
        b.bids = std::move(bids);
        b.asks = std::move(asks);
        return b;
    }

	TradeTick trade(double price, double qty, bool isBuy)
    {
        TradeTick t;
        t.price = price;
        t.qty = qty;
        t.isBuy = isBuy;
        return t;
    }
    struct Scenario
    {
        OrderBookSnapshot prev;
        OrderBookSnapshot current;
        Trades trades;
        MarketEventLog* log = nullptr;
        qint64 timestamp = 100;

        OrderBookFeatureRow run()const
        {
            AggressiveIcebergDetector detector;
            DetectionContext ctx;
            ctx.prevBook = &prev;
            ctx.hasPrevBook = true;
            ctx.currentBook = &current;
            ctx.currentTimestamp = timestamp;
            ctx.tickTolerance = kTick;
            ctx.pendingTrades = &trades;
            ctx.eventLog = log;

            OrderBookFeatureRow row;
            detector.detect(ctx, row);
            return row;
        }
    };
}


class AggressiveIcebergDetectorTest : public QObject
{
    Q_OBJECT

private slots:
    void detect_bidSweep_isDetected();
    //void detect_askSweep_idDetected();
    //void detect_partialFill_isNotSweep();
    //void detect_cancelWithoutTrades_isNotSweep();
    //void detect_wronfAggressorSide_isNotSweep();
    //void detect_chainBreaks_countsOnlyConsecutive();
    //void detect_singleLevelSweep_siNotRecorded();
    //void detect_nullEventLog_doesNotCrash();
    //void detect_repeateSweep_countsPreviousOnes();
};

void AggressiveIcebergDetectorTest::detect_bidSweep_isDetected()
{
    Scenario s;

    s.prev = book(
  {
            { 100.0, 5.0 },
            { 99.9, 4.0 },
            { 99.8, 3.0 },
            { 99.7, 2.0 }
        },
  {
            { 100.1, 5.0 },
            { 100.2, 4.0 },
            { 100.3, 3.0 }
       });
	s.current = book(
        {
        	{ 99.7, 2.0 }
         },                                         // три верхних бида исчезли
   {
   			{ 100.1, 5.0 },
   			{ 100.2, 4.0 },
   			{ 100.3, 3.0 }
		});
	s.trades = 
    {
		trade(100.0, 5.0, false),
		trade(99.9, 4.0, false),
		trade(99.8, 3.0, false) 
	};
    const OrderBookFeatureRow row = s.run();
    QCOMPARE(row.bidSweepLevels, 3);
    QCOMPARE(row.bidSweepQty, 12.0f);
    QCOMPARE(row.askSweepLevels, 0);
    QCOMPARE(row.askSweepQty, 0.0f);

}
QTEST_APPLESS_MAIN(AggressiveIcebergDetectorTest)
#include "AggressiveIcebergDetectorTest.moc"