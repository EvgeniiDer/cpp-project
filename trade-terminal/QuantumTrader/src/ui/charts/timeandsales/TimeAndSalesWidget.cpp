#include "TimeAndSalesWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <algorithm>
#include <cmath>

TimeAndSalesWidget::TimeAndSalesWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(1);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void TimeAndSalesWidget::addTick(const TradeTick& tick)
{
    m_ticks.push_front(tick);           // новые — сверху
    if (m_ticks.size() > MAX_TICKS)
        m_ticks.pop_back();
    if (m_scrollOffset == 0)            // если не скроллили — обновляем вид
        update();
}

void TimeAndSalesWidget::clear()
{
    m_ticks.clear();
    m_scrollOffset = 0;
    update();
}

// ─────────────────────────────────────────────────────────────────────────────

void TimeAndSalesWidget::resizeEvent(QResizeEvent* event)
{
    float w         = static_cast<float>(event->size().width());
    float remaining = w - m_colTimeWidth - m_colPriceWidth;
    if (!m_sizePinned)
        m_colSizeWidth = std::max(0.0f, remaining);
    else
        m_colSizeWidth = std::max(0.0f, std::min(m_colSizeWidth, remaining));
    QWidget::resizeEvent(event);
}

void TimeAndSalesWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(0, 0, width(), height(), m_bgColor);

    drawHeader(p);

    if (m_ticks.empty()) return;

    int total    = static_cast<int>(m_ticks.size());
    int maxOff   = std::max(0, total - 1);
    m_scrollOffset = std::clamp(m_scrollOffset, 0, maxOff);

    float y = m_headerHeight;

    QFont f = p.font();
    f.setPixelSize(12);
    p.setFont(f);

    for (int i = m_scrollOffset; i < total && y < height(); ++i)
    {
        const TradeTick& tick = m_ticks[i];
        QRectF rowRect(0, y, width(), m_rowHeight);

        // Фоновая подсветка строки
        p.fillRect(rowRect, tick.isBuy ? m_buyBg : m_sellBg);

        const QColor& clr = tick.isBuy ? m_buyColor : m_sellColor;

        // Time
        p.setPen(m_textColor);
        p.drawText(QRectF(4, y, m_colTimeWidth - 4, m_rowHeight),
                   Qt::AlignVCenter | Qt::AlignLeft, tick.time);

        // Price
        p.setPen(clr);
        p.drawText(QRectF(m_colTimeWidth, y, m_colPriceWidth - 4, m_rowHeight),
                   Qt::AlignVCenter | Qt::AlignRight, formatPrice(tick.price));

        // Size
        p.setPen(m_textColor);
        p.drawText(QRectF(m_colTimeWidth + m_colPriceWidth, y, m_colSizeWidth - 4, m_rowHeight),
                   Qt::AlignVCenter | Qt::AlignRight, formatQty(tick.qty));

        y += m_rowHeight;
    }
}

void TimeAndSalesWidget::drawHeader(QPainter& p)
{
    QRectF hdr(0, 0, width(), m_headerHeight);
    p.fillRect(hdr, m_headerBg);

    QFont f = p.font();
    f.setPixelSize(11);
    p.setFont(f);
    p.setPen(m_headerColor);

    p.drawText(QRectF(4, 0, m_colTimeWidth - 4,  m_headerHeight),
               Qt::AlignVCenter | Qt::AlignLeft,  "Time");
    p.drawText(QRectF(m_colTimeWidth, 0, m_colPriceWidth - 4, m_headerHeight),
               Qt::AlignVCenter | Qt::AlignRight, "Price");
    p.drawText(QRectF(m_colTimeWidth + m_colPriceWidth, 0, m_colSizeWidth - 4, m_headerHeight),
               Qt::AlignVCenter | Qt::AlignRight, "Size");

    // Разделители
    p.setPen(QPen(m_sepColor, 2));
    auto sep = [&](float x) {
        int xi = static_cast<int>(x);
        p.drawLine(xi, 3, xi, static_cast<int>(m_headerHeight) - 3);
    };
    sep(m_colTimeWidth);
    sep(m_colTimeWidth + m_colPriceWidth);
    if (m_colSizeWidth > 0.0f)
        sep(m_colTimeWidth + m_colPriceWidth + m_colSizeWidth);
}

// ─────────────────────────────────────────────────────────────────────────────
// Mouse / Wheel
// ─────────────────────────────────────────────────────────────────────────────

int TimeAndSalesWidget::hitTestSeparator(int x) const
{
    auto near = [&](float sx) {
        return std::abs(x - static_cast<int>(sx)) <= SEP_HIT;
    };
    if (near(m_colTimeWidth))                                         return 0;
    if (near(m_colTimeWidth + m_colPriceWidth))                       return 1;
    if (near(m_colTimeWidth + m_colPriceWidth + m_colSizeWidth))      return 2;
    return -1;
}

void TimeAndSalesWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) { QWidget::mousePressEvent(event); return; }
    int x   = static_cast<int>(event->position().x());
    int col = hitTestSeparator(x);
    if (col >= 0)
    {
        m_dragging       = true;
        m_dragCol        = col;
        m_dragStartX     = x;
        m_dragWidthStart = (col == 0) ? m_colTimeWidth
                         : (col == 1) ? m_colPriceWidth
                         :              m_colSizeWidth;
        setCursor(Qt::SizeHorCursor);
    }
    event->accept();
}

void TimeAndSalesWidget::mouseMoveEvent(QMouseEvent* event)
{
    int x = static_cast<int>(event->position().x());
    if (!m_dragging)
    {
        setCursor(hitTestSeparator(x) >= 0 ? Qt::SizeHorCursor : Qt::ArrowCursor);
        return;
    }

    constexpr float MIN_COL = 5.0f;
    float delta   = static_cast<float>(x - m_dragStartX);
    float totalW  = static_cast<float>(width());

    if (m_dragCol == 0)
    {
        float maxT = totalW - m_colPriceWidth - m_colSizeWidth - MIN_COL;
        m_colTimeWidth = std::clamp(m_dragWidthStart + delta, MIN_COL, maxT);
    }
    else if (m_dragCol == 1)
    {
        float maxP = totalW - m_colTimeWidth - m_colSizeWidth - MIN_COL;
        m_colPriceWidth = std::clamp(m_dragWidthStart + delta, MIN_COL, maxP);
    }
    else
    {
        float maxS = totalW - m_colTimeWidth - m_colPriceWidth;
        m_colSizeWidth = std::clamp(m_dragWidthStart + delta, MIN_COL, maxS);
        m_sizePinned   = true;
    }
    update();
    event->accept();
}

void TimeAndSalesWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
    else { QWidget::mouseReleaseEvent(event); }
}

void TimeAndSalesWidget::wheelEvent(QWheelEvent* event)
{
    int steps = -event->angleDelta().y() / 120;
    steps *= (event->modifiers() & Qt::ShiftModifier) ? 1 : 3;
    m_scrollOffset = std::clamp(m_scrollOffset + steps, 0,
                                std::max(0, static_cast<int>(m_ticks.size()) - 1));
    update();
    event->accept();
}

// ─────────────────────────────────────────────────────────────────────────────

QString TimeAndSalesWidget::formatPrice(double price)
{
    if (price >= 10000.0) return QString::number(price, 'f', 1);
    if (price >= 100.0)   return QString::number(price, 'f', 2);
    if (price >= 1.0)     return QString::number(price, 'f', 4);
    return QString::number(price, 'f', 6);
}

QString TimeAndSalesWidget::formatQty(double qty)
{
    if (qty >= 1'000'000.0) return QString::number(qty / 1'000'000.0, 'f', 2) + "M";
    if (qty >= 1'000.0)     return QString::number(qty / 1'000.0,     'f', 2) + "K";
    return QString::number(qty, 'f', 4);
}
