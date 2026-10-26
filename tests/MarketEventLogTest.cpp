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
	bool writeTextFile(const QString& path, const QByteArray& content)
	{
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			return false;
		}
		return file.write(content) == content.size();
	}

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
	void load_validFile_loadsEvents();
	void load_emptyLines_areSkipped();
	void load_wrongFileCount_lineIsSkipped();
	void load_badNumber_lineIsSkipped();
	void load_unknownType_behavesAsDecided();
	void load_oldEvents_evictedAsDecided();
};

////--- constructor
void MarketEventLogTest::load_oldEvents_evictedAsDecided()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath("events.csv");

	QVERIFY(writeTextFile(path,
		"1000,bid,sweep,100,1\n" // слишком старое
		"99000,bid,sweep,100,1\n"        // ровно на границе, остаётся
		"100000,bid,sweep,100,1\n"));

	MarketEventLog log(path, 1000);

	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 2);
}

void MarketEventLogTest::load_badNumber_lineIsSkipped()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath("events.csv");

	QVERIFY(writeTextFile(path, 
		"abc,bid,sweep,100,1\n"        // время не число
		"1500,bid,sweep,xyz,1\n"       // цена не число
		"1600,bid,sweep,100,oops\n"    // объём не число
		"2000,bid,sweep,100,1\n"));

	MarketEventLog log(path, kDay);

	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 1);
}

void MarketEventLogTest::load_unknownType_behavesAsDecided()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath("events.csv");

	QVERIFY(writeTextFile(path,
		"1000,bid,foo,100,1\n" // неизвестный тип: строка должна быть пропущена
		"2000,bid,sweep,100,1\n"));

	MarketEventLog log(path, kDay);
	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 1);
}

void MarketEventLogTest::load_wrongFileCount_lineIsSkipped()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath("events.csv");
	QVERIFY(writeTextFile(path,
		"1000,bid,sweep,100\n"     // 4 поля: не хватает объёма
		"1500,bid,sweep,100,1,extra\n"    // 6 полей: лишнее поле
		"2000,bid,sweep,100,1\n"));

	MarketEventLog log(path, kDay);

	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 1);
}

void MarketEventLogTest::load_emptyLines_areSkipped()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath("events.csv");

	QVERIFY(writeTextFile(path, "1000,bid,sweep,100,1\n"
		"\n"
		"   \n"
		"\n"                                                    
		"2000,bid,sweep,100,1\n"
		"\n"));
	MarketEventLog log(path, kDay);
	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 2);
}

void MarketEventLogTest::load_validFile_loadsEvents()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	const QString path = dir.filePath("events.csv");

	QVERIFY(writeTextFile(path, "1000,bid,sweep,100,1\n"	"2000,ask,spoof,200.5,2\n" "3000,bid,sweep,100,1\n"));

	MarketEventLog log(path, kDay);

	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 2);
	QCOMPARE(log.countNear(MarketEventType::SpoofBurst, QStringLiteral("ask"), 200.5, kZoneTolerance), 1);
	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("ask"), 200.5, kZoneTolerance), 0);
}

void MarketEventLogTest::load_missingFile_givesEmptyLog()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	const QString path = dir.filePath("events.csv");
	QVERIFY(!QFile::exists(path));

	MarketEventLog log(path, kDay);

	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("bid"), 100.0, kZoneTolerance), 0);

	QCOMPARE(log.countNear(MarketEventType::Sweep, QStringLiteral("ask"), 100.0, kZoneTolerance), 0);
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