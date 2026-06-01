#pragma once


#include<QMatrix4x4>
#include<QPointF>
#include<QPainter>

namespace chart
{
	enum class DragState
	{
		None,
		ChartArea,
		PriceAxis,
		DateAxis,
		ResizePriceAxis
	};
	struct Viewport
	{
		float priceMin;
		float priceMax;
		float candleIndexMin;
		float candleIndexMax;

		float width() const { return candleIndexMax - candleIndexMin; }
		float height() const { return priceMax - priceMin; }
	};

	struct ChartSettings
	{
		float priceAxisWidth = 60.0f;
		float timeAxisHeight = 30.0f;
		int initialCandleCount = 500;// Первоночальная загрузка свечей!!!! Можно в будущем сделать как настройку для юзера

	};
	struct ChartContext
	{
	    const QMatrix4x4& mvpMatrix;
	    const Viewport& viewport;

	    QPointF mouseChartPos;
	    bool isMouseOver;

	    QPainter* painter = nullptr;
	    int widgetWidth = 0;
	    int widgetHeight = 0;

    // Сюда прокидываем размеры из настроек для удобства слоев
	   float priceAxisWidth = 0.0f;
	   float timeAxisHeight = 0.0f;
	};
	struct CameraStat
	{
		float x = 0.0f;
		float y = 0.0f;
		float zoomX = 50.0f;
		float zoomY = 100.0f;
	};
}
