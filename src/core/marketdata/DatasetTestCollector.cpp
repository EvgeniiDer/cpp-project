#include"DatasetTestCollector.h"
#include"OrderBookCsvExporter.h"
#include"core/events/EventBus.h"
#include"core/managers/MarketDataManager.h"
#include<QDateTime>
#include<QDebug>
#include<QDir>
DatasetTestCollector::DatasetTestCollector(MarketDataManager* dataManager, const QString& exchange, const QString& symbol, QObject* parent)
	:QObject(parent)
	,m_dataManager(dataManager)
	,m_exchange(exchange)
	,m_symbol(symbol)
	,m_collector(5, 5LL * 60 * 1000, this)
{
	double tickSize = m_dataManager ? m_dataManager->getTickSize(exchange, symbol) : 0.0;
	m_collector.watch(exchange, symbol, tickSize);
	QObject::connect(&EventBus::instance(), &EventBus::orderBookReceived, this, &DatasetTestCollector::onOrderBookReceived, Qt::UniqueConnection);
	QObject::connect(&EventBus::instance(), &EventBus::instrumentTickSizesLoaded, this, &DatasetTestCollector::onTickSizesLoaded, Qt::UniqueConnection);
	QObject::connect(&m_exportTimer, &QTimer::timeout, this, &DatasetTestCollector::onExportTimer);
	m_exportTimer.start(60 * 1000);
	qDebug() << "[DatasetTestCollector] started for" << exchange << symbol;

	MarketContext ctx;
	ctx.exchange = exchange;
	ctx.symbol = symbol;
	ctx.marketType = "PERP";
	ctx.streamType = StreamType::OrderBook;
	if (m_dataManager)
	{
		m_dataManager->subscribeToStream(ctx);
	}
	MarketContext tradesCtx;
	tradesCtx.exchange = exchange;
	tradesCtx.symbol = symbol;
	tradesCtx.marketType = "PERP";
	tradesCtx.streamType = StreamType::Trades;
	if (m_dataManager)
	{
		m_dataManager->subscribeToStream(tradesCtx);
	}
	qDebug() << "[DatasetTestCollector] working dir:" << QDir::currentPath();
}

void DatasetTestCollector::onOrderBookReceived(const QString& exchange, const QString& symbol, const OrderBookSnapshot& snapshot, bool isDelta)
{
	if (exchange != m_exchange || symbol != m_symbol)
	{
		return;
	}
	if (isDelta)
	{
		m_book.applyDelta(snapshot);
	}
	else
	{
		m_book.setData(snapshot);
	}
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	m_collector.recordState(now, m_book.snapshot());
}
void DatasetTestCollector::onTickSizesLoaded(const QString& exchange, const QHash<QString, double>& tickSizes)
{
	if (exchange != m_exchange)
	{
		return;
	}
	if (tickSizes.contains(m_symbol))
	{
		m_collector.watch(m_exchange, m_symbol, tickSizes.value(m_symbol));
		qDebug() << "[DatasetTestCollector] tickSize updated:  " << tickSizes.value(m_symbol);
	}
}
void DatasetTestCollector::onExportTimer()
{
	QVector<OrderBookFeatureRow> rows = m_collector.takeHistory();
	qDebug() << "[DatasetTestCollector] rows in window: " << rows.size();

	if (rows.size() < 2)
	{
		return;
	}
	const QString path = QString("dataset_test_%1.csv").arg(++m_exportCounter);
	bool ok = OrderBookCsvExporter::exportToCsv(rows, path, 60 * 1000);
	qDebug() << "[DatasetTestCollector] export" << path << (ok ? "Ok" : "FAILED");
}


