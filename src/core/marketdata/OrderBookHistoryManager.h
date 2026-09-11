#pragma once
#include<QObject>
#include<QString>
#include<QVector>
#include<QDateTime>
#include<deque>
#include"types/OrderBookDataTypes.h"

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
	// Сколько уровней (из top-N) на этой стороне ЗА ЭТОТ КАДР дозаправились
	// после реальной проторговки (см. detectIcebergRefill).
	int bidIcebergRefillCount = 0;
	int askIcebergRefillCount = 0;

	//Сумарный обьем дозаправки айсберга 
	float bidIcebergRefillQty = 0.0f;
	float askIcebergRefillQty = 0.0f;
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
	struct SpoofFeatures
	{
		float bidSpoofDrop = 0.0f;
		float askSpoofDrop = 0.0f;
		float bidFakeRatio = 0.0f;
		float askFakeRati0 = 0.0f;
	};

	struct IcebergFeatures
	{
		int bidRefillCount = 0;
		int askRefillCount = 0;

		float bidRefillQty = 0.0f;
		float askRefillQty = 0.0f;
	};
	struct IcebergLevelState
	{
		double lastQty = 0.0;
		qint64 lastSeenTimestamp = 0;
		bool waitingRefill = false;
		bool initialized = false;
	};
	// Кандидат на "айсберг мог переставиться на соседний тик" — живёт
	// недолго (короткое временное окно), пока не будет либо использован
	// (нашли дозаправку рядом), либо не устареет.
	struct PendingIcebergDepletion
	{
		qint64 tickKey = 0;
		qint64 timestamp = 0;
		double qty = 0.0;
	};
	// Насколько далеко (в тиках) ищем "переставленный" айсберг от места,
	// где он только что реально проторговался изменямое
	static constexpr qint64 kIcebergWalkTicks = 3;

	// Сколько времени кандидат остаётся "в поиске" пары, прежде чем считать,
	// что дозаправки не будет и он просто исчез навсегда.
	static constexpr  qint64 kIcebergWalkWindowMs = 200;

	void evicOldRows(qint64 currentTimestamp);
	float calcDepthImbalance(const OrderBookSnapshot& book)const;

	// Спуфинг: сравнение ТОЛЬКО топ-уровня (bids[0]/asks[0]) между двумя
	// Смысл менять глубину на несколько снэпшотов нету так как интересны изменения толкьо последних два снэпшота
	// соседними снепшотами. Чистая функция — ничего не меняет в состоянии класса.
	SpoofFeatures detectSpoofing(const OrderBookSnapshot& prevBook, const OrderBookSnapshot& currentBook, double bestBid, double bestAsk, double tickTolerance)const;

	IcebergFeatures detectIcebergRefill(const OrderBookSnapshot& currentBook, qint64 currentTimestamp, double tickTolerance);

	// Обработка одной стороны стакана (общий код для бида и аска, чтобы
	// не дублировать один и тот же цикл дважды).
	void processIcebergSide(const std::vector<OrderBookLevel>& levels, std::map<qint64, IcebergLevelState>& state, qint64 currentTimestamp, double tickTolerance, int& refillCount, float& refillQty);

	// Убирает из карты айсберг-состояний уровни, которые давно не обновлялись
	// (цена ушла далеко, уровень выпал из top-N) — иначе карта растёт вечно.
	void evictStaleIceberLevels(quint64 currentTimestamp);

	// Перевод цены в целочисленный "тик" для использования как ключ std::map —
	// сравнивать double на равенство в контейнере ненадёжно (плавающая точка),
	// а тики с округлением дают стабильный и быстрый ключ.
	qint64 priceToTickKey(double price)const;

	int m_depthLevels;
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

	// Состояние айсберг-детектора — отдельно для бида и аска, ключ — тик цены.
	std::map<qint64, IcebergLevelState> m_bidIcebergLevels;
	std::map<qint64, IcebergLevelState> m_askIcebergLevels;

	bool m_tickSizeWarned = false;
};
