#pragma once
#include<QObject>
#include<QString>
#include<QVector>
#include<QDateTime>
#include<deque>

#include "detectors/IOrderBookFeatureDetector.h"
#include"types/OrderBookDataTypes.h"
#include"types/OrderBookFeatureRow.h"

namespace OrderBookConfig
{
	constexpr qint64 kDefaultRetentionMs = 7LL * 24 * 3600 * 1000; //1 неделя
	//Пресеты
	constexpr qint64 kOneWeekMs = 7LL * 24 * 3600 * 1000;
	constexpr qint64 kTwoWeeksMs = 14LL * 24 * 3600 * 1000;
	constexpr qint64 kOneMonthMs = 30LL * 24 * 3600 * 1000;
	constexpr qint64 kThreeMonthsMs = 90LL * 24 * 3600 * 1000;
}
class OrderBookHistoryManager: public QObject
{
	Q_OBJECT
public:
	explicit OrderBookHistoryManager(int depthLevels = 5, qint64 retentionsMs = OrderBookConfig::kDefaultRetentionMs, QObject* parent = nullptr);
	void watch(const QString& exchange, const QString& symbol, double tickSize);
	void recordState(qint64 currentTimestamp, const OrderBookSnapshot& currentBook);
	QVector<OrderBookFeatureRow>takeHistory()const;
	void clearHistory();
public slots:
	void onTradeReceived(const QString& exchange, const QString& symbol, const TradeTick& tick);

private:
	void evicOldRows(qint64 currentTimestamp);
	float calcDepthImbalance(const OrderBookSnapshot& book)const;

	int m_depthLevels;
	qint64 m_retentionMs;
	std::deque<OrderBookFeatureRow> m_historyRows;

	OrderBookSnapshot m_prev1;
	OrderBookSnapshot m_prev2;

	bool m_hasPrev1 = false;
	bool m_hasPrev2 = false;

	QString m_watchedExchange;
	QString m_watchedSymbol;
	double m_tickSize = 0.0;
	std::vector<TradeTick>m_pendingTrades;

	bool m_tickSizeWarned = false;

	std::vector<std::unique_ptr<IOrderBookFeatureDetector>> m_detectors;
};
