#include <QtTest>
#include <windows.h>

#include"core/marketdata/detectors/AggressiveIcebergDetector.h"

namespace
{
    constexpr double kTick = 0.1;
    constexpr int kDay = 24LL * 60 * 60 * 1000;
    constexpr double kZoneTolerance = kTick * 3.5;
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
    OrderBookSnapshot baseBook()
    {
        return book
        (
			{
				{100.0, 5.0},
				{99.9, 4.0},
				{99.8, 3.0},
				{99.7, 2.0},
			},
			{
				{100.1, 5.0},
				{100.2, 4.0},
				{100.3, 3.0},
				{100.4, 2.0}
			}
        );
    }
    Scenario bidSweepFrom(double topPrice, MarketEventLog* log)
    {
        Scenario s;
        s.log = log;
        s.prev = book(
            {
                {topPrice, 5.0},
                {topPrice - 0.1, 4.0},
                {topPrice - 0.2, 3.0},
                {topPrice - 0.4, 2.0}
            },
            baseBook().asks
        );
        s.current = book(
            {
                {topPrice - 0.2, 3.0},
                {topPrice - 0.3, 2.0}
            },
            baseBook().asks
        );
        s.trades = {
	        trade(topPrice, 5.0, false),
            trade(topPrice - 0.1, 4.0, false)
        };
        return s;
    }
}


class AggressiveIcebergDetectorTest : public QObject
{
    Q_OBJECT

private slots:
    void detect_repeatSweep_narrowAndWideZones();
    void detect_bidSweep_isDetected();
    void detect_askSweep_isDetected();
    void detect_partialFill_isNotSweep();
    void detect_cancelWithoutTrades_isNotSweep();
    void detect_wrongAggressorSide_isNotSweep();
    void detect_chainBreaks_countsOnlyConsecutive();
    void detect_singleLevelSweep_isNotRecorded();
    void detect_nullEventLog_doesNotCrash();
    void detect_repeatSweep_oldEventsAreForgotten();
    void detect_explainedRatio_threshold_data();
    void detect_explainedRatio_threshold();
    void detect_askRepeatSweep_countsSeparatelyFromBids();
};


void AggressiveIcebergDetectorTest::detect_chainBreaks_countsOnlyConsecutive()
{
    Scenario s;
    s.prev = baseBook();
    s.current = baseBook();
    s.current.asks.erase(s.current.asks.begin(), s.current.asks.begin() + 2);
    s.current.asks[0].price = 100.3;
    s.current.asks[0].qty = 2.0;

    s.trades = {
        trade(100.1, 5.0, true),
        trade(100.2, 4.0, true),
        trade(100.3, 1.0, true)
    };

    OrderBookFeatureRow row = s.run();

    QCOMPARE(row.bidSweepLevels, 0);
    QCOMPARE(row.bidSweepQty, 0.0f);
    QCOMPARE(row.askSweepLevels, 2);
    QCOMPARE(row.askSweepQty, 9.0f);
}

void AggressiveIcebergDetectorTest::detect_singleLevelSweep_isNotRecorded()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    MarketEventLog log(dir.filePath("events.csv"), kDay);

    Scenario s;
    s.log = &log;
    s.prev = baseBook();
    s.current = baseBook();

    s.current.bids.erase(s.current.bids.begin(), s.current.bids.begin() + 1);
    s.trades = {
	    trade(100.0, 5.0, false)
    };
    const OrderBookFeatureRow row = s.run();

    QCOMPARE(row.bidSweepLevels, 1);
    QCOMPARE(row.bidSweepQty, 5.0f);
    QCOMPARE(row.bidSweepRepeatCount, 0);
    QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 0);

}

void AggressiveIcebergDetectorTest::detect_nullEventLog_doesNotCrash()
{
    Scenario s;
    s.prev = baseBook();
    s.current = baseBook();
    s.current.bids.erase(s.current.bids.begin(), s.current.bids.begin() + 2);

    s.trades = {
        trade(100.0, 5.0, false),
        trade(99.9, 4.0, false)
    };
    const OrderBookFeatureRow row = s.run();

    QCOMPARE(row.bidSweepLevels, 2);
    QCOMPARE(row.bidSweepQty, 9.0f);
    QCOMPARE(row.bidSweepRepeatCount, 0);
}

void AggressiveIcebergDetectorTest::detect_repeatSweep_oldEventsAreForgotten()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    MarketEventLog log(dir.filePath("events.csv"), 1000);

    Scenario s;
    s.log = &log;
    s.prev = baseBook();
    s.current = baseBook();
    s.current.bids.erase(s.current.bids.begin(), s.current.bids.begin() + 2);
    s.trades = {
        trade(100.0, 5.0, false),
        trade(99.9, 4.0, false)
    };
    s.timestamp = 100;
    const OrderBookFeatureRow first = s.run();
    QCOMPARE(first.bidSweepLevels, 2);
    QCOMPARE(first.bidSweepRepeatCount, 0);

    s.timestamp = 200;
    const OrderBookFeatureRow second = s.run();
    QCOMPARE(second.bidSweepRepeatCount, 1);

    s.timestamp = 300;
    const OrderBookFeatureRow third = s.run();
    QCOMPARE(third.bidSweepRepeatCount, 2);

    s.timestamp = 1400;
    const OrderBookFeatureRow four = s.run();
    QCOMPARE(four.bidSweepRepeatCount, 0);

    s.timestamp = 1500;
    const OrderBookFeatureRow five = s.run();
    QCOMPARE(five.bidSweepRepeatCount, 1);

    QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 2);

    s.timestamp = 5000;
    const OrderBookFeatureRow six = s.run();
    QCOMPARE(six.bidSweepRepeatCount, 0);

    QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 1);

}

void AggressiveIcebergDetectorTest::detect_explainedRatio_threshold_data()
{
    QTest::addColumn<double>("tradedQty");
    QTest::addColumn<int>("expectedLevels");
    QTest::addColumn<float>("expectedQty");

    QTest::newRow("exactlyAtThreshold") << 4.5 << 1 << 4.5f;
    QTest::newRow("aboveThreshold") << 4.6 << 1 << 4.6f;
    QTest::newRow("belowThreshold") << 4.49 << 0 << 0.0f;
    QTest::newRow("farBelowThreshold") << 4.0 << 0 << 0.0f;
}

void AggressiveIcebergDetectorTest::detect_explainedRatio_threshold()
{
    QFETCH(double, tradedQty);
    QFETCH(int, expectedLevels);
    QFETCH(float, expectedQty);

    Scenario s;
    s.prev = baseBook();
    s.current = baseBook();
    s.current.bids.erase(s.current.bids.begin(), s.current.bids.begin() + 1);

    s.trades = {
        trade(100.0, tradedQty, false)
    };

    const OrderBookFeatureRow row = s.run();

    QCOMPARE(row.bidSweepLevels, expectedLevels);
    QCOMPARE(row.bidSweepQty, expectedQty);
}
void AggressiveIcebergDetectorTest::detect_askRepeatSweep_countsSeparatelyFromBids()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MarketEventLog log(dir.filePath("events.csv"), kDay);

    Scenario asks;
    asks.log = &log;
    asks.prev = baseBook();
    asks.current = baseBook();
    asks.current.asks.erase(asks.current.asks.begin(), asks.current.asks.begin() + 2);
	asks.trades = {
		trade(100.1, 5.0, true),
        trade(100.2, 4.0, true)
	};
    asks.timestamp = 100;
    const OrderBookFeatureRow first = asks.run();
    QCOMPARE(first.askSweepLevels, 2);
    QCOMPARE(first.askSweepRepeatCount, 0);

    asks.timestamp = 200;
    const OrderBookFeatureRow second = asks.run();
    QCOMPARE(second.askSweepRepeatCount, 1);

    Scenario bids;
    bids.log = &log;
    bids.prev = baseBook();
    bids.current = baseBook();
    bids.current.bids.erase(bids.current.bids.begin(), bids.current.bids.begin() + 2);
	bids.trades = {
		trade(100.0, 5.0, false),
        trade(99.9, 4.0, false)
	};
    bids.timestamp = 300;
    const OrderBookFeatureRow bidRow = bids.run();
    QCOMPARE(bidRow.bidSweepLevels, 2);
    QCOMPARE(bidRow.bidSweepRepeatCount, 0);
}


void AggressiveIcebergDetectorTest::detect_partialFill_isNotSweep()
{
    Scenario s;
    s.prev = baseBook();
    s.current = baseBook();
    s.current.bids[0].qty = 2.0;

    s.trades = {
	    trade(100.0, 3.0, false)
    };

    const OrderBookFeatureRow row = s.run();

    QCOMPARE(row.bidSweepLevels, 0);
    QCOMPARE(row.bidSweepQty, 0.0f);
    QCOMPARE(row.askSweepLevels, 0);
    QCOMPARE(row.askSweepQty, 0.0f);
}

void AggressiveIcebergDetectorTest::detect_cancelWithoutTrades_isNotSweep()
{
    Scenario s;
    s.prev = baseBook();
    s.current = baseBook();
    s.current.bids.erase(s.current.bids.begin(), s.current.bids.begin() + 1);

    const OrderBookFeatureRow row = s.run();

    QCOMPARE(row.bidSweepLevels, 0);
    QCOMPARE(row.bidSweepQty, 0.0f);
    QCOMPARE(row.askSweepLevels, 0);
    QCOMPARE(row.askSweepQty, 0.0f);
}

void AggressiveIcebergDetectorTest::detect_wrongAggressorSide_isNotSweep()
{
    Scenario s;
    s.prev = baseBook();
    s.current = baseBook();
    s.current.bids.erase(s.current.bids.begin(), s.current.bids.begin() + 3);


    s.trades = {
        trade(100.0, 5.0, true),
		trade(99.9, 4.0, true),
        trade(99.8, 3.0, true)
	};

    const OrderBookFeatureRow row = s.run();

    QCOMPARE(row.bidSweepLevels, 0);
    QCOMPARE(row.bidSweepQty, 0.0f);
    QCOMPARE(row.askSweepLevels, 0);
    QCOMPARE(row.askSweepQty, 0.0f);

}

void AggressiveIcebergDetectorTest::detect_repeatSweep_narrowAndWideZones()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MarketEventLog log(dir.filePath("events.csv"), kDay);
    Scenario s = bidSweepFrom(100.0, &log);
    s.timestamp = 100;
    const OrderBookFeatureRow first = s.run();
    QCOMPARE(first.bidSweepRepeatCount, 0);
    QCOMPARE(first.bidSweepRepeatCountNarrow, 0);

    s = bidSweepFrom(99.7, &log);
    s.timestamp = 200;
    const OrderBookFeatureRow second = s.run();
    QCOMPARE(second.bidSweepRepeatCount, 1);
    QCOMPARE(second.bidSweepRepeatCountNarrow, 0);

    s = bidSweepFrom(100.0, &log);
    s.timestamp = 300;
    const OrderBookFeatureRow third = s.run();
    QCOMPARE(third.bidSweepRepeatCount, 2);
    QCOMPARE(third.bidSweepRepeatCountNarrow, 1);
}

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

void AggressiveIcebergDetectorTest::detect_askSweep_isDetected()
{
    Scenario s;
    s.prev = baseBook();
    s.current = baseBook();
    s.current.asks.erase(s.current.asks.begin(), s.current.asks.begin() + 3);

    s.trades = {
	    trade(100.1, 5.0, true),
        trade(100.2, 4.0, true),
        trade(100.3, 3.0, true)
    };

    const OrderBookFeatureRow row = s.run();

    QCOMPARE(row.askSweepLevels, 3);
    QCOMPARE(row.askSweepQty, 12.0f);
    QCOMPARE(row.bidSweepLevels, 0);
    QCOMPARE(row.bidSweepQty, 0.0f);


}



QTEST_APPLESS_MAIN(AggressiveIcebergDetectorTest)
#include "AggressiveIcebergDetectorTest.moc"