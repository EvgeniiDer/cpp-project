#include"CrosshairLayer.h"
#include<QPainter>

CrosshairLayer::CrosshairLayer()
{
}
void CrosshairLayer::paintUI(const chart::ChartContext& context)
{
	if (!context.isMouseOver || !context.painter) return;
	QPainter* p = context.painter;


	float mx = context.mouseChartPos.x();
	float my = context.mouseChartPos.y();

	float availableWidth = context.widgetWidth - context.priceAxisWidth;
	float availableHeight = context.widgetHeight - context.timeAxisHeight;

	if (mx >= availableWidth || my > availableHeight) return;

	p->save();

	QPen crossPen(QColor(180, 180, 180, 150));
	crossPen.setStyle(Qt::DashLine);
	p->setPen(crossPen);

	p->drawLine(QPoint(mx, 0), QPointF(mx, availableHeight));
	
	p->drawLine(QPoint(0, my), QPointF(availableWidth, my));

	const chart::Viewport& vp = context.viewport;

	float normalizedY = (context.widgetHeight - my) / context.widgetHeight;
	float priceARange = vp.priceMax - vp.priceMin;
	float priceAtMouse = vp.priceMin + (normalizedY * priceARange);

	QString priceStr = QString::number(priceAtMouse, 'f', 2);
	p->setPen(Qt::NoPen);
	p->setBrush(QColor(40, 45, 50));
	p->drawRect(availableWidth, my - 10, context.priceAxisWidth, 20);

	p->setPen(Qt::white);
	p->drawText(availableWidth + 5, my + 4, priceStr);

	float normalizedX = mx / availableWidth;
	float indexRange = vp.candleIndexMax - vp.candleIndexMin;
	float indexAtMouse = vp.candleIndexMin + (normalizedX * indexRange);

	QDateTime baseDate = QDateTime::currentDateTime();
	QDateTime hoverDate = baseDate.addDays(static_cast<int>(indexAtMouse));
	QString timeStr = hoverDate.toString("dd MMM HH:mm");

	p->setPen(Qt::NoPen);
	p->setBrush(QColor(40, 45, 50));
	p->drawRect(mx - 35, availableHeight, 70, context.timeAxisHeight);

	p->setPen(Qt::white);
	p->drawText(mx - 30, context.widgetHeight - 8, timeStr);

	p->restore(); }
