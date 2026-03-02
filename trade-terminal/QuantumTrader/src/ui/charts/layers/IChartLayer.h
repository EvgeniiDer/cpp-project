#pragma once

#include<QOpenGlFunctions>
#include"../ChartTypes.h"

class IChartLayer : protected QOpenGLFunctions
{
public:
	virtual ~IChartLayer() = default;
	virtual void initializeGL() = 0;
	virtual void paintGL(const chart::ChartContext& context) = 0;
	virtual void paintUI(const chart::ChartContext& context) {};
	virtual void resizeGL(int w, int h) {};// Слоям не обязательно реагировать на изменение размера к примеру основного окна, но если нужно - то можно переопределить
	virtual void cleanup() {};
	
};