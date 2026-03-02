#pragma once

#include"IChartLayer.h"
#include"QColor"
#include"QFont"
#include<QDateTime>


class AxisLayer : public IChartLayer
{
public:
	AxisLayer();
	~AxisLayer() override {}
	void initializeGL() override{}
	void paintGL(const chart::ChartContext& context)override{}

	void paintUI(const chart::ChartContext& constext)override;
	void setTextColor(const QColor& color);
private:
	QColor m_textColor = QColor(200, 200, 200);
	QFont m_font;
};
