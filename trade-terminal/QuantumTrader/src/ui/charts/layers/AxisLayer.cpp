#include"AxisLayer.h"
#include<QPainter>
#include<cmath>



AxisLayer::AxisLayer()
{
	m_font.setFamily("Arial");
	m_font.setPixelSize(11);
}
void AxisLayer::paintUI(const chart::ChartContext& context)
{
	QPainter* p = context.painter;

	if (!p)return;

	const auto& vp = context.viewport;

	p->setFont(m_font);
	p->setPen(m_textColor);
	

	p->fillRect(QRectF(QPointF(context.widgetWidth - context.priceAxisWidth, 0),QSizeF( context.priceAxisWidth, context.widgetHeight)), QColor(20, 25, 30, 200));

    float rangeY = vp.height();
    if (rangeY <= 0.00001f) rangeY = 1.0f;

    float stepY = std::pow(10.0f, std::floor(std::log10(rangeY / 5.0f)));
    if (stepY <= 0.00001f) stepY = 1.0f;
    float startY = std::floor(vp.priceMin / stepY) * stepY;

    // Цикл по ценам
    for (float y = startY; y <= vp.priceMax; y += stepY)
    {
        // Переводим цену (y) в пиксели на экране
        // В Qt координата Y=0 - это ВЕРХ окна, поэтому считаем "снизу вверх"
        float normalizedY = (y - vp.priceMin) / rangeY;
        int pixelY = context.widgetHeight - (normalizedY * context.widgetHeight);

        // Форматируем число (2 знака после запятой)
        QString text = QString::number(y, 'f', 2);

        // Рисуем текст справа
        p->drawText(context.widgetWidth - context.priceAxisWidth + 5, pixelY + 4, text);
    }
    //Подложка снизу там где время 
    p->fillRect(0, context.widgetHeight - context.timeAxisHeight, context.widgetWidth, context.timeAxisHeight, QColor(20, 25, 30, 200));

    float rangeX = vp.width();
    float availableWidth = context.widgetWidth - context.priceAxisWidth;

    int maxLabels = std::max(1, static_cast<int>(availableWidth / 50.0f));

    float minStepX = rangeX / maxLabels;

    float power = std::pow(10.0f, std::floor(std::log10(minStepX > 0 ? minStepX : 1.0f)));
    float stepX = power;
    if (minStepX > power * 5.0f) stepX = power * 10.0f;
    else if (minStepX > power * 2.0f) stepX = power * 5.0f;
    else if (minStepX > power) stepX = power * 2.0f;
    
    if (stepX < 1.0f) stepX = 1.0f;

    float startX = std::floor(vp.candleIndexMin / stepX) * stepX;

    QDateTime baseDate = QDateTime::currentDateTime();

    for (float x = startX; x <= vp.candleIndexMax; x += stepX)
    {
        // Переводим индекс (x) в пиксели экрана (используем availableWidth!)
        float normalizedX = (x - vp.candleIndexMin) / rangeX;
        int pixelX = normalizedX * availableWidth;

        // Если дата улетела за правую панель цен - не рисуем её
        if (pixelX > availableWidth || pixelX < 0) continue;

        // Превращаем индекс в дату (считаем, что 1 свеча = 1 день)
        QDateTime labelDate = baseDate.addDays(static_cast<int>(x));

        // Форматируем красиво: "dd MMM" даст нам "01 Мар", "15 Апр" и т.д.
        QString text = labelDate.toString("dd MMM");

        // Рисуем текст снизу (сдвигаем на 20 пикселей влево, чтобы отцентрировать)
        p->drawText(pixelX - 20, context.widgetHeight - 8, text);
    }

}