#pragma once

#include<QOpenGlFunctions>
#include<QMatrix4x4>
#include<QPointF>

//Обьект параметра цены и времени
struct Viewport
{
	float priceMin;
	float priceMax;
	float candleIndexMin;
	float candleIndexMax;

	float width() const { return candleIndexMax - candleIndexMin; }
	float height() const { return priceMax - priceMin; }
};
//Обьект параметр матрицы и вьюпорта для отрисовки слоя
struct ChartContext
{
	const QMatrix4x4& mvpMatrix; // Матрица для преобразования координат графика в координаты OpenGL (to MVP)
	const Viewport& viewport;// Границы поля для отрисовки

	QPointF mouseChartPos; // Позиция мышки в координатах графика (не пикселях, а в терминах цены и времени)
	bool isMouseOver;//Мышка над графиком или нет

	QPainter* painter = nullptr;
	int widgetWidth = 0;
	int widgetHeight = 0;
};

class IChartLayer : protected QOpenGLFunctions
{
public:
	virtual ~IChartLayer() = default;
	virtual void initializeGL() = 0;
	virtual void paintGL(const ChartContext& context) = 0;
	virtual void paintUI(const ChartContext& context) {};
	virtual void resizeGL(int w, int h) {};// Слоям не обязательно реагировать на изменение размера к примеру основного окна, но если нужно - то можно переопределить
	virtual void cleanup() {};
	
};