#pragma once
#include"../types/OrderBookDataTypes.h"
#include"../types/OrderBookFeatureRow.h"
// prevBook       - предыдущий снепшот стакана (m_prev1 из OrderBookHistoryManager).
//                  Может быть "пустым" (дефолтным) на первом кадре; проверяйте hasPrevBook.
// hasPrevBook    - есть ли валидный prevBook (аналог m_hasPrev1).
// currentBook    - текущий снепшот стакана.
// currentTimestamp - метка времени текущего кадра (мс).
// tickTolerance  - допуск по цене для сравнения (см. m_tickSize).
// outRow         - сюда детектор ЗАПИСЫВАЕТ свои поля (bidSpoofDrop и т.д.).
//                  Остальные поля outRow трогать нельзя: они уже заполнены
//                  до вызова детектора либо будут заполнены другим детектором.

struct DetectionContext
{
	const OrderBookSnapshot* prevBook = nullptr;
	bool hasPrevBook = false;
	const OrderBookSnapshot* currentBook = nullptr;
	qint64 currentTimestamp = 0;
	double tickTolerance = 0.0;
	const std::vector<TradeTick>* pendingTrades = nullptr;
	int depthLevels = 5;
};


// Общий контракт для всех детекторов признаков стакана (спуфинг, айсберг,
// и любые будущие). OrderBookHistoryManager хранит их как
// std::vector<std::unique_ptr<IOrderBookFeatureDetector>> и просто
// вызывает detect() по очереди, не зная, что конкретно внутри каждого.
class IOrderBookFeatureDetector
{
public:
	virtual ~IOrderBookFeatureDetector() = default;
	
	// ctx    - входные данные (см. DetectionContext выше).
	// outRow — куда детектор ЗАПИСЫВАЕТ результат (остальные поля не трогает).
	virtual void detect(const DetectionContext& ctx, OrderBookFeatureRow& outRow) = 0;
	[[nodiscard]] virtual QString name()const = 0;
};
