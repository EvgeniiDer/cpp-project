#pragma once
#include<QObject>
#include<QTimer>
#include<QString>
#include<QHash>
#include"OrderBookHistoryManager.h"
#include"ui/charts/orderbook/OrderBookModel.h"

class MarketDataManager;

class DatasetTestCollector : public QObject
{
	Q_OBJECT
public:
	explicit DatasetTestCollector(MarketDataManager* dataManager, const QString& exchange, const QString& symbol, QObject* parent = nullptr);
public slots:
	void onOrderBookReceived(const QString& exchange, const QString& symbol, const OrderBookSnapshot& snapshot, bool isDelta);
	void onTickSizesLoaded(const QString& exchange, const QHash<QString, double>& tickSizes);
	void onExportTimer();
private:
	MarketDataManager* m_dataManager;
	QString m_exchange;
	QString m_symbol;

	OrderBookModel m_book;
	OrderBookHistoryManager m_collector;
	QTimer m_exportTimer;
	int m_exportCounter = 0;
};