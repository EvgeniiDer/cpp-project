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
	//void countNear_priceExactlyAtTolerance_isCounted();

	////--- record
	//void record_event_isVisibleInCountNear();
	//void record_event_survivesReload();
	//void record_price_keepsPrecisionAfterReload();
	//void record_oldEvent_isEvictedByNewer();
	//void record_eventExactlyAtRetentionBorder_isKept();

	////-- constructor

	//void load_missingFile_givesEmptyLog();
	//void load_validFile_loadsEvents();
	//void load_emptyLines_areSkipped();
	//void load_wrongFileCount_lineIsSkipped();
	//void laod_badNumber_lineIsSkipped();
	//void load_unknownType_behavesAsDecided();
	//void load_oldEvents_evictedAsDecided();
};

void MarketEventLogTest::countNear_priceOutsideTolerance_isIgnored()
{

}

void MarketEventLogTest::countNear_otherSide_isIgnored()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	MarketEventLog log(dir.filePath("event.csv"), kDay);

	log.record(marketEvent(QStringLiteral("bid"), 300.0));
	log.record(marketEvent(QStringLiteral("ask"), 300.0));

	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 300.0, kZoneTolerance), 1);
	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("ask"), 300.0, kZoneTolerance), 1);
}

void MarketEventLogTest::countNear_otherType_isIgnored()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	MarketEventLog log(dir.filePath("events.csv"), kDay);

	log.record(marketEvent(QStringLiteral("bid"), 300.0, MarketEventType::SpoofBurst));
	log.record(marketEvent(QStringLiteral("bid"), 300.0, MarketEventType::Sweep));

	QCOMPARE(log.countNear(MarketEventType::SpoofBurst, QStringLiteral("bid"), 300.0, kZoneTolerance), 1);
	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 300.0, kZoneTolerance), 1);
}

void MarketEventLogTest::countNear_severalMatching_allCounted()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	MarketEventLog log(dir.filePath("events.csv"), kDay);

	log.record(marketEvent(QStringLiteral("bid"), 300.0));
	log.record(marketEvent(QStringLiteral("bid"), 300.0));
	log.record(marketEvent(QStringLiteral("bid"), 300.0));
	log.record(marketEvent(QStringLiteral("bid"), 300.0));
	log.record(marketEvent(QStringLiteral("bid"), 300.0));

	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 300.0, kZoneTolerance), 5);
}

void MarketEventLogTest::countNear_emptyLog_isZero()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	MarketEventLog log(dir.filePath("events.csv"), kDay);

	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 0);
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