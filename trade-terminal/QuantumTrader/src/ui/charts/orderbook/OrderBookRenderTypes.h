#pragma once
#include<QColor>
#include"OrderBookTypes.h"


enum class OrderBookViewMode
{
	Classic,
	Cluster,
};

struct VolumeCircle
{
	double qty = 0.0; //Volume
	double maxQty = 0.0; 
	bool isAsk = false;
// Нормализация радиуса не было проблем с маленьки и большим радиуосом 
	float normalizedRadius() const
	{
		if (maxQty <= 0) return 0.0f;
		return static_cast<float>(std::min(1.0, qty / maxQty));
	}
};
struct RowContext
{
	const OrderBookLevel& level;
	double maxTotal = 0.0;
	bool isAsk = false;

	VolumeCircle circle;
};

struct OrderBookRenderConfig
{
	OrderBookViewMode viewMode = OrderBookViewMode::Classic;
	float circleColW = 40.f;
	// Сюда в будущем:
	// bool  showCumulative = false;
	// bool  showDelta      = false;   // дельта bid/ask давление
	// int   heatmapLevels  = 0;       // тепловая карта объёмов
};