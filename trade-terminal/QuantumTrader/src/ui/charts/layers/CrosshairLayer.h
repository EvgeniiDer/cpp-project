#pragma once
#include"IChartLayer.h"
#include<QPen>
#include<QBrush>
#include<QDateTime>
class CrosshairLayer : public IChartLayer
{
private:
public:
	CrosshairLayer();
	~CrosshairLayer() override = default;
	void initializeGL() override {}
	void paintGL(const chart::ChartContext& context) override{}
	void paintUI(const chart::ChartContext& context) override;
};