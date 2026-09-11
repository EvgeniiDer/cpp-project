#pragma once
#include"IOrderBookFeatureDetector.h"
#include<map>
#include<deque>

// Детектор дозаправки айсберга - зеркальная логика спуфингу: объём упал,
// но ОБЪЯСНЁН реальными сделками (traded ~ drop), а не пропал в никуда.
// Если ПОСЛЕ этого на том же ценовом уровне (или на соседнем тике - см.
// "walking" ниже) объём снова вырос - это дозаправка скрытого айсберга.
//
// В отличие от SpoofDetector - этот класс СО СОСТОЯНИЕМ (stateful):
// он должен помнить состояние уровня ДОЛЬШЕ одного кадра (дозаправка может
// случиться не сразу на следующем обновлении), поэтому detect() не const
// и класс хранит карты между вызовами.
class IcebergDetector : public IOrderBookFeatureDetector
{
public:
	void detect(const DetectionContext& ctx, OrderBookFeatureRow& outRow) override;
	[[nodiscard]]QString name() const override;

	void reset()override;
private:
	// double lastQty последний известный объем на этом уровне
	// qint64 lastSeenTimestamp  время последнего обновления уровня (мс)
	// bool awaitingRefill ждем ли мы возврата объема(дозаправки)
	// bool initialized  был ли уровень уже инициализирован (первая встреча)
	struct IcebergLevelState
	{
		double lastQty = 0.0;
		qint64 lastSeenTimestamp = 0;
		bool awaitingRefill = false;
		bool initialized = false;
	};
	// "Билет" на недавнее РЕАЛЬНОЕ исполнение заявки (не спуф) на каком-то
	// уровне - висит короткое время (kIcebergWalkWindowMs), пока мы ждём,
	// не появится ли объём обратно рядом (переставленный айсберг).
	// Если не появился вовремя - запись просто протухает и удаляется.
	struct PendingIcebergDepletion
	{
		// Цена уровня, где произошло исполнение - округлённая до тика
		// (результат priceToTickKey), а не "сырая" цена с плавающей точкой.
		// Нужна как ключ для сравнения "далеко ли" соседний уровень
		// (через std::llabs(a - b) <= kIcebergWalkTicks).
		qint64 priceTickKey = 0;

		// Когда была создана эта запись (currentTimestamp на момент постановки
		// уровня "в очередь ожидания"). По ней проверяем, не истёк ли срок
		// ожидания дозаправки: currentTimestamp - depletionTimestamp > kIcebergWalkWindowMs.
		qint64 depletionTimestamp = 0;

		// Сколько объёма реально проторговалось в этот момент (traded).
		// Сейчас нигде не читается при поиске совпадения - записано "про
		// запас", если позже понадобится фильтр "дозаправка похожего
		// размера" (не путать мелкую случайную покупку с крупным айсбергом).
		double tradedQty = 0.0;
	};

	struct  IcebergSideState
	{
		std::map<qint64, IcebergLevelState>* levelStates = nullptr;
		std::deque<PendingIcebergDepletion>* pendingDepletion = nullptr;
	};

	struct IcebergSideResult
	{
		int refillCount = 0; //колличестов дозаправок не обьем а сколько раз тоесть каждый раз когда происходит дозаправка инкримент
		float refillQty = 0;// а это уже колличество в обьеме сколько бло увеличино дозаправок
	};

	IcebergSideResult processSide(const std::vector<OrderBookLevel>& bookLevels, IcebergSideState sideState, const  DetectionContext& ctx);
	void evictStaleLevels(const qint64 currentTimestamp);
	// Перевод цены в целочисленный тик для ключа std::map - см. обсуждение
	// про ненадёжность сравнения double как ключа контейнера.
    [[nodiscard]]static qint64 priceToTickKey(const double price,const double tickSize);
	// Насколько ticks в сторону ищем "переставленный" айсберг.
	static constexpr qint64 kIcebergWalkTicks = 3;
	// Сколько мс кандидат остаётся "в поиске" пары.
	static constexpr qint64 kIcebergWalkWindowMs = 2000;
	// Порог traded/drop, после которого считаем drop "реальной проторговкой".
	static constexpr double kExplainedRatioThreshold = 0.9;
	// Окно устаревания записей в state-картах (не обновлялись - вычищаем).
	static constexpr qint64 kLevelRetentionMs = 5LL * 60 * 1000;

	std::map<qint64, IcebergLevelState> m_bidLevels;
	std::map<qint64, IcebergLevelState> m_askLevels;
	std::deque<PendingIcebergDepletion> m_bidPendingDepletions;
	std::deque<PendingIcebergDepletion> m_askPendingDepletions;
};
