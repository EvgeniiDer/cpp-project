#pragma once
#include"IOrderBookFeatureDetector.h"


// Детектор спуфинга: сравнивает ТОЛЬКО топ-уровень (bids[0]/asks[0]) между
// двумя соседними снепшотами (ctx.prevBook -> ctx.currentBook). Смотреть
// глубже нет смысла - интересны изменения именно последних двух снепшотов
// на лучшей цене, см. разбор формулы drop/traded/unexplained/fakeRatio
// в старой реализации OrderBookHistoryManager::recordState.
//
// Класс без состояния (stateless) - ничего не запоминает между вызовами
// detect(), в отличие от IcebergDetector. Поэтому не требует ни полей,
// ни конструктора - вся логика укладывается в один метод.

class SpoofDetector : public IOrderBookFeatureDetector
{
public:
	void detect(const DetectionContext& ctx, OrderBookFeatureRow& outRow) override;
	QString name()const override;
};