#include<QtTest>
#include"core/marketdata/detectors/DetectorMath.h"
#include"core/marketdata/log/MarketEventLog.h"
namespace
{
	constexpr qint64 kDay = 24LL * 3600 * 1000;
	constexpr double kTick = 0.1;
	constexpr double kZoneTolerance = kTick * 3.5;
	MarketEvent marketEvent(const QString& side, double price, MarketEventType type = MarketEventType::Sweep, qint64 ts = 1000)
	{
		MarketEvent e;
		e.timestamp = ts;
		e.side = side;
		e.price = price;
		e.type = type;
		e.magnitude = 1.0f;
		return e;
	}
	struct TempLog
	{
		QTemporaryDir dir;
		MarketEventLog log;
		explicit TempLog(qint64 retention = kDay) : log(dir.filePath("events.csv"), retention){}
	};

}

class MarketEventLogTest : public QObject
{
	Q_OBJECT
private slots:
	// ---countNear
	void countNear_emptyLog_isZero();
	void countNear_matchingEvent_isCounted();
	void countNear_severalMatching_allCounted();
	void countNear_otherType_isIgnored();
	void countNear_otherSide_isIgnored();
	void countNear_priceOutsideTolerance_isIgnored();
	void countNear_priceExactlyAtTolerance_isCounted();

	////--- record
	void record_event_isVisibleInCountNear();
	void record_event_survivesReload();
	void record_price_keepsPrecisionAfterReload();
	void record_oldEvent_isEvictedByNewer();
	void record_retentionBorder_keepsBorderEventsAndEvictsOlder();

	////-- constructor

	void load_missingFile_givesEmptyLog();
	//void load_validFile_loadsEvents();
	//void load_emptyLines_areSkipped();
	//void load_wrongFileCount_lineIsSkipped();
	//void laod_badNumber_lineIsSkipped();
	//void load_unknownType_behavesAsDecided();
	//void load_oldEvents_evictedAsDecided();
};

////--- constructor
void MarketEventLogTest::load_missingFile_givesEmptyLog()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	const QString path = dir.filePath("events.csv");
	QVERIFY(!QFile::exists(path));

	MarketEventLog(path, kDay);
}

////--- record

void MarketEventLogTest::record_retentionBorder_keepsBorderEventsAndEvictsOlder()
{
	TempLog t(1000);
	QVERIFY(t.dir.isValid());
	t.log.record(marketEvent(QStringLiteral("bid"), 100.0, MarketEventType::Sweep, 3999));
	t.log.record(marketEvent(QStringLiteral("bid"), 100.0, MarketEventType::Sweep, 4000));
	t.log.record(marketEvent(QStringLiteral("bid"), 100.0, MarketEventType::Sweep, 4000));
	t.log.record(marketEvent(QStringLiteral("bid"), 100.0, MarketEventType::Sweep, 5000));

	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, DetectorMath::kPriceEpsilon), 3);
}

void MarketEventLogTest::record_oldEvent_isEvictedByNewer()
{
	TempLog t(1000);
	QVERIFY(t.dir.isValid());

	t.log.record(marketEvent(QStringLiteral("bid"), 100.0, MarketEventType::Sweep, 1000));
	t.log.record(marketEvent(QStringLiteral("bid"), 100.0, MarketEventType::Sweep, 5000));
	t.log.record(marketEvent(QStringLiteral("bid"), 100.0, MarketEventType::Sweep, 345));

	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 2);
}

void MarketEventLogTest::record_price_keepsPrecisionAfterReload()
{
	TempLog t;
	QVERIFY(t.dir.isValid());

	constexpr double price = 45654.2453232343132;

	t.log.record(marketEvent(QStringLiteral("bid"), price));

	MarketEventLog reloaded(t.dir.filePath("events.csv"), kDay);

	QCOMPARE(reloaded.countNear(MarketEventType::Sweep, QStringLiteral("bid"), price, DetectorMath::kPriceEpsilon), 1);
}

void MarketEventLogTest::record_event_survivesReload()
{
	TempLog t;
	QVERIFY(t.dir.isValid());

	t.log.record(marketEvent(QStringLiteral("bid"), 100.0));
	MarketEventLog reloaded(t.dir.filePath("events.csv"), kDay);

	QCOMPARE(reloaded.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 1);
}

void MarketEventLogTest::record_event_isVisibleInCountNear()
{
	TempLog t;
	QVERIFY(t.dir.isValid());

	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 0);
	t.log.record(marketEvent(QStringLiteral("bid"), 100.0));

	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 1);
}

//--countNear
void MarketEventLogTest::countNear_priceExactlyAtTolerance_isCounted()
{
	TempLog t;
	QVERIFY(t.dir.isValid());

	t.log.record(marketEvent(QStringLiteral("bid"), 100.0));

	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.5, 0.4999), 0);
	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.5, 0.5), 1);
}

void MarketEventLogTest::countNear_priceOutsideTolerance_isIgnored()
{
	TempLog t;
	QVERIFY(t.dir.isValid());
	constexpr double base = 300.0;
	t.log.record(marketEvent(QStringLiteral("bid"), base + 4 * kTick));
	t.log.record(marketEvent(QStringLiteral("bid"), base + 3 * kTick));

	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), base, kZoneTolerance), 1);
}

void MarketEventLogTest::countNear_otherSide_isIgnored()
{
	TempLog t;
	QVERIFY(t.dir.isValid());

	t.log.record(marketEvent(QStringLiteral("bid"), 300.0));
	t.log.record(marketEvent(QStringLiteral("ask"), 300.0));

	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 300.0, kZoneTolerance), 1);
	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("ask"), 300.0, kZoneTolerance), 1);
}

void MarketEventLogTest::countNear_otherType_isIgnored()
{
	TempLog t;
	QVERIFY(t.dir.isValid());

	t.log.record(marketEvent(QStringLiteral("bid"), 300.0, MarketEventType::SpoofBurst));
	t.log.record(marketEvent(QStringLiteral("bid"), 300.0, MarketEventType::Sweep));

	QCOMPARE(t.log.countNear(MarketEventType::SpoofBurst, QStringLiteral("bid"), 300.0, kZoneTolerance), 1);
	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 300.0, kZoneTolerance), 1);
}

void MarketEventLogTest::countNear_severalMatching_allCounted()
{
	TempLog t;
	QVERIFY(t.dir.isValid());
	t.log.record(marketEvent(QStringLiteral("bid"), 300.0));
	t.log.record(marketEvent(QStringLiteral("bid"), 300.0));
	t.log.record(marketEvent(QStringLiteral("bid"), 300.0));
	t.log.record(marketEvent(QStringLiteral("bid"), 300.0));
	t.log.record(marketEvent(QStringLiteral("bid"), 300.0));

	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 300.0, kZoneTolerance), 5);
}

void MarketEventLogTest::countNear_emptyLog_isZero()
{
	TempLog t;
	QVERIFY(t.dir.isValid());
	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 0);
}

void MarketEventLogTest::countNear_matchingEvent_isCounted()
{
	TempLog t;
	QVERIFY(t.dir.isValid());
	t.log.record(marketEvent(QStringLiteral("bid"), 250.5));

	QCOMPARE(t.log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 250.5, kZoneTolerance), 1);
}

QTEST_APPLESS_MAIN(MarketEventLogTest)
#include "MarketEventLogTest.moc"