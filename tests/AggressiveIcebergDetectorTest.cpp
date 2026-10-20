#include <QtTest>

class AggressiveIcebergDetectorTest : public QObject
{
    Q_OBJECT

private slots:
    void smoke_buildAndRun()
    {
        QVERIFY(true);
    }
};

QTEST_APPLESS_MAIN(AggressiveIcebergDetectorTest)
#include "AggressiveIcebergDetectorTest.moc"