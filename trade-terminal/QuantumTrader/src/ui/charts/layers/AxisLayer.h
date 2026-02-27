#pragma once

#include"IChartLayer.h"
#include"QColor"
#include"QFont"


class AxisLayer : public IChartLayer
{
public:
	AxisLayer();
	~AxisLayer() override {}
	void initializeGL() override{}
	void paintGL(const ChartContext& context)override{}

	void paintUI(const ChartContext& constext)override;
	void setTextColor(const QColor& color);
	int getPriceAxisWidth()const;
	int getTimeAxisHeight()const;
	void setPriceAxisWidth(int newWidth);
	void setTimeAxisHeight(int height);
private:
	QColor m_textColor = QColor(200, 200, 200);
	QFont m_font;

	int m_priceAxisWidth = 50;
	int m_timeAxisHeight = 55;
};
