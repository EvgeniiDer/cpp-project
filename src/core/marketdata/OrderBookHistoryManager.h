#pragma once
#include<QObject>
#include<QString>
#include<QVector>
#include<QDateTime>
#include<deque>
#include"OrderBookDataTypes.h"

struct OrderBookFeatureRow
{
	qint64 timestamp = 0;
	double bestBid = 0.0;
	double bestAsk = 0.0;
	double spread = 0.0;
	float imbalanceL1 = 0.0f;
	float imbalanceSma3 = 0.0f;
	float bidSpoofDrop = 0.0f;
	float askSpoofDrop = 0.0f;
	float bidFakeRatio = 0.0f;
	float askFakeRatio = 0.0f;
	float volumeDepthBids = 0.0f;
	float volumeDepthAsks = 0.0f;
	double midPrice = 0.0;

	bool isValid = false;
};
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
	/**
	 * \brief Настраивает менеджер на слежение за указанным торговым инструментом.
	 *
	 * Данный метод устанавливает биржу, символ и точность цены (ticksize шаг цены) для сбора и анализа данных.
	 * При вызове очищается буфер ожидающих сделок (m_pendingTrades), что гарантирует, что данные
	 * от предыдущего инструмента не повлияют на новый.
	 *
	 * Используется как для первоначальной инициализации, так и для переключения между инструментами
	 * без пересоздания объекта.
	 *
	 * \param exchange  Название биржи (например, "Bybit").
	 * \param symbol    Торговый символ (например, "BTCUSDT").
	 * \param tickSize  Минимальный шаг цены для данного инструмента. Используется при сравнении цен
	 *                  в методах анализа (например, sumTradedQtyNearPrice).
	 *
	 * \note Если tickSize равен 0.0, в методах анализа будет использоваться запасное значение
	 *       (kPriceEpsilon * 100.0) — это временное решение, чтобы код не падал.
	 * \warning Метод не проверяет валидность переданных параметров; caller должен убедиться,
	 *          что exchange и symbol корректны, а tickSize получен из достоверного источника.
	 */
	void watch(const QString& exchange, const QString& symbol, double tickSize);
	void recordState(qint64 currentTimestamp, const OrderBookSnapshot& currentBook);
	QVector<OrderBookFeatureRow>takeHistory()const;
	void clearHistory();
public slots:
	void onTradeReceived(const QString& exchange, const QString& symbol, const TradeTick& tick);

private:
	void evicOldRows(qint64 currentTimestamp);
	float calcDepthImbalance(const OrderBookSnapshot& book)const;

	int m_detpthLevels;
	qint64 m_retentionMs;
	std::deque<OrderBookFeatureRow>m_historyRows;

	OrderBookSnapshot m_prev1;
	OrderBookSnapshot m_prev2;

	bool m_hasPrev1 = false;
	bool m_hasPrev2 = false;

	QString m_watchedExchange;
	QString m_watchedSymbol;
	double m_tickSize = 0.0;
	std::vector<TradeTick> m_pendingTrades;
};
