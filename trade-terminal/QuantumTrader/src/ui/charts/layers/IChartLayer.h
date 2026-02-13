#pragma once

#include<QOpenGlFunctions>
#include<QMatrix4x4>
#include<QPointF>


struct Viewport
{
	float priceMin;
	float priceMax;
	float candleIndexMin;
	float candleIndexMax;

	float width() const { return candleIndexMax - candleIndexMin; }
	float height() const { return priceMax - priceMin; }
};

struct ChartContext
{
	const QMatrix4x4& mvpMatrix;
	const Viewport& viewport;

	QPointF mouseChartPos;
	bool isMouseOver;
};

class IChartLayer : protected QOpenGLFunctions
{
public:
	virtual ~IChartLayer() = default;
	virtual void initializeGL() = 0;
	virtual void paintGL(const ChartContext& context) = 0;
	virtual void resizeGL(int w, int h) {};
	virtual void cleanup() {};
	
};