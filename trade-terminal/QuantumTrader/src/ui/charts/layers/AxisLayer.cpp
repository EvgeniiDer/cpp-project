#include"AxisLayer.h"
#include<QPainter>
#include<cmath>



AxisLayer::AxisLayer()
{
	m_font.setFamily("Arial");
	m_font.setPixelSize(11);
}


void AxisLayer::setTextColor(const QColor& color)
{
	m_textColor = color;
}

void AxisLayer::paintUI(const ChartContext& context)
{
	QPainter* p = context.painter;

	if (!p)return;

	const auto& vp = context.viewport;

	p->setFont(m_font);
	p->setPen(m_textColor);
	

	p->fillRect(QRectF(QPointF(context.widgetWidth - m_priceAxisWidth, 0),QSizeF( m_priceAxisWidth, context.widgetHeight)), QColor(20, 25, 30, 200));

    p->fillRect(context.widgetWidth - m_priceAxisWidth, 0,
        m_priceAxisWidth, context.widgetHeight, QColor(20, 25, 30, 200));

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
        p->drawText(context.widgetWidth - m_priceAxisWidth + 5, pixelY + 4, text);
    }

    // --- 2. НИЖНЯЯ ОСЬ (ВРЕМЯ / ИНДЕКС) ---
    // Рисуем подложку снизу
    p->fillRect(0, context.widgetHeight - m_timeAxisHeight,
        context.widgetWidth, m_timeAxisHeight, QColor(20, 25, 30, 200));

    float rangeX = vp.width();
    float stepX = std::max(1.0f, std::pow(10.0f, std::floor(std::log10(rangeX / 8.0f))));
    float startX = std::floor(vp.candleIndexMin / stepX) * stepX;

    // Цикл по индексам свечей
    for (float x = startX; x <= vp.candleIndexMax; x += stepX)
    {
        // Переводим индекс (x) в пиксели экрана
        float normalizedX = (x - vp.candleIndexMin) / rangeX;
        int pixelX = normalizedX * context.widgetWidth;

        // Пока что выводим просто индекс свечи (Потом заменим на Дату/Время)
        QString text = QString::number(x, 'f', 0);

        // Рисуем текст снизу
        p->drawText(pixelX - 10, context.widgetHeight - 8, text);
    }

}