#pragma once
#include <QWidget>
#include <deque>
#include "TimeAndSalesDataTypes.h"

class QPainter;

/**
 * @brief Виджет ленты сделок (Time & Sales).
 *
 * Рисует список исполненных сделок — новые сверху, уходят вниз.
 * Зелёный = покупка (Buy), красный = продажа (Sell).
 *
 * Колонки: Time | Price | Size
 * Каждая колонка тянется разделителем (как в OrderBook).
 */
class TimeAndSalesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimeAndSalesWidget(QWidget* parent = nullptr);

    void addTick(const TradeTick& tick);
    void clear();

protected:
    void paintEvent(QPaintEvent* event)   override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event)   override;
    void mouseMoveEvent(QMouseEvent* event)    override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event)        override;

private:
    void drawHeader(QPainter& p);
    int  hitTestSeparator(int x) const;

    static QString formatPrice(double price);
    static QString formatQty(double qty);

    // ── Данные ─────────────────────────────────────────────────────
    static constexpr int MAX_TICKS = 500;
    std::deque<TradeTick> m_ticks;

    // ── Layout ─────────────────────────────────────────────────────
    float m_rowHeight    = 20.0f;
    float m_headerHeight = 24.0f;

    float m_colTimeWidth  = 70.0f;
    float m_colPriceWidth = 80.0f;
    float m_colSizeWidth  = 70.0f;   // Total-like: заполняет остаток пока не запинован
    bool  m_sizePinned    = false;

    int m_scrollOffset = 0;

    // ── Цвета ──────────────────────────────────────────────────────
    QColor m_bgColor     { 0x14, 0x16, 0x1a };
    QColor m_headerBg    { 0x1f, 0x22, 0x28 };
    QColor m_buyColor    { 0x0e, 0xcb, 0x81 };
    QColor m_sellColor   { 0xf6, 0x46, 0x5d };
    QColor m_buyBg       { 0x0e, 0xcb, 0x81, 25 };
    QColor m_sellBg      { 0xf6, 0x46, 0x5d, 25 };
    QColor m_textColor   { 0xd1, 0xd5, 0xdb };
    QColor m_headerColor { 0x6b, 0x72, 0x80 };
    QColor m_sepColor    { 0xff, 0xff, 0xff };

    // ── Drag ───────────────────────────────────────────────────────
    static constexpr int SEP_HIT = 4;
    bool  m_dragging       = false;
    int   m_dragCol        = -1;
    int   m_dragStartX     = 0;
    float m_dragWidthStart = 0.0f;
};
