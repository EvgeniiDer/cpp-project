#include "OrderBookClassic.h"
#include <QPainter>
#include <QResizeEvent>
#include <algorithm>
#include <cmath>

OrderBookClassic::OrderBookClassic(QWidget* parent)
    : OrderBookBase(parent)
{
    setMinimumWidth(1);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void OrderBookClassic::resizeEvent(QResizeEvent* event)
{
    float w         = static_cast<float>(event->size().width());
    float remaining = w - m_columnPriceWidth - m_columnQtyWidth;

    if (!m_totalPinned)
        m_columnTotalWidth = std::max(0.0f, remaining);      // до первого drag-а — заполняет всё
    else
        m_columnTotalWidth = std::max(0.0f,                  // после drag-а — фиксированный, но не выходит за границу
                                      std::min(m_columnTotalWidth, remaining));
    QWidget::resizeEvent(event);
}

/**
 * @brief Единый прокручиваемый список: все аски (высокая цена → лучший аск)
 *        затем спред, затем все биды (лучший бид → низкая цена).
 *
 * Виртуальный список строк:
 *   row 0 … nAsks-1 : asks[nAsks-1-row]  (высокая→низкая цена)
 *   row nAsks        : СПРЕД
 *   row nAsks+1 …   : bids[row - nAsks - 1]  (высокая→низкая цена)
 *
 * m_scrollOffset = индекс первой видимой строки.
 * Колёсиком вверх: offset-- (видим более высокие аски).
 * Колёсиком вниз:  offset++ (видим более глубокие биды).
 */
void OrderBookClassic::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(0, 0, width(), height(), m_colors.bgColor);

    drawHeader(p, QRect(0, 0, width(), static_cast<int>(m_headerHeight)));

    const auto& snap = m_model.snapshot();
    const int nAsks  = static_cast<int>(snap.asks.size());
    const int nBids  = static_cast<int>(snap.bids.size());
    if (nAsks == 0 && nBids == 0) return;

    // Полный виртуальный список: nAsks строк + 1 спред + nBids строк
    const int totalRows = nAsks + 1 + nBids;
    clampScrollOffset(totalRows);

    const double maxAskTotal = OrderBookModel::calcMaxTotal(snap.asks);
    const double maxBidTotal = OrderBookModel::calcMaxTotal(snap.bids);

    float y = m_headerHeight;

    for (int row = m_scrollOffset; row < totalRows && y < height(); ++row)
    {
        if (row < nAsks)
        {
            // ── Ask строка ──────────────────────────────────────────
            // row 0 = asks[nAsks-1] (самый дорогой), row nAsks-1 = asks[0] (лучший аск)
            int idx = nAsks - 1 - row;
            VolumeCircle vc{ snap.asks[idx].qty, snap.asks[0].qty, true };
            RowContext    ctx{ snap.asks[idx], maxAskTotal, true, vc };
            drawRow(p, QRectF(0, y, width(), m_rowHeight), ctx);
            y += m_rowHeight;
        }
        else if (row == nAsks)
        {
            // ── Спред ───────────────────────────────────────────────
            drawSpread(p, QRect(0, static_cast<int>(y), width(), static_cast<int>(m_spreadHeight)));
            y += m_spreadHeight;
        }
        else
        {
            // ── Bid строка ──────────────────────────────────────────
            // row nAsks+1 = bids[0] (лучший бид), далее вглубь
            int idx = row - nAsks - 1;
            VolumeCircle vc{ snap.bids[idx].qty, snap.bids[0].qty, false };
            RowContext    ctx{ snap.bids[idx], maxBidTotal, false, vc };
            drawRow(p, QRectF(0, y, width(), m_rowHeight), ctx);
            y += m_rowHeight;
        }
    }
}
