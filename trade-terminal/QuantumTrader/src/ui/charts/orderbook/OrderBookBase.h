#pragma once
#include <QWidget>
#include <QColor>
#include "OrderBookDataTypes.h"
#include "OrderBookRenderTypes.h"
#include "OrderBookModel.h"

class QPainter;

/**
 * @brief Базовый класс всех видов стакана.
 *
 * Отвечает за:
 *  - владение моделью данных (OrderBookModel)
 *  - публичный API: setData / applyDelta / setDepth / snapshot
 *  - общий набор примитивов отрисовки (drawHeader, drawSpread, drawRow, drawVolumeCircle)
 *  - общую палитру цветов и параметры layout (rowHeight, colWidths…)
 *  - скролл колёсиком мыши (m_scrollOffset): единый список asks→spread→bids
 *  - ресайз колонок мышью: тяни границу между колонками
 *
 * Каждый конкретный вид (Classic, Cluster, …) наследуется от OrderBookBase
 * и реализует только paintEvent + resizeEvent со своей компоновкой.
 */
class OrderBookBase : public QWidget
{
    Q_OBJECT
public:
    explicit OrderBookBase(QWidget* parent = nullptr);
    ~OrderBookBase() override = default;

    void setData(const OrderBookSnapshot& snapshot);
    void applyDelta(const OrderBookSnapshot& delta);
    void setDepth(int rows);
    const OrderBookSnapshot& snapshot() const { return m_model.snapshot(); }

protected:
    // ── Общие draw-примитивы ───────────────────────────────────────
    void drawHeader(QPainter& p, const QRect& rect);
    void drawSpread(QPainter& p, const QRect& rect);
    void drawRow(QPainter& p, const QRectF& rect, const RowContext& ctx);
    void drawVolumeCircle(QPainter& p, const QPointF& center, float radius,
                          double qty, bool isAsk, bool showLabel = true);

    static QString formatPrice(double price);
    static QString formatQty(double qty);
    static QString formatTotal(double total);

    // ── Mouse / wheel ──────────────────────────────────────────────
    void mousePressEvent(QMouseEvent* event)   override;
    void mouseMoveEvent(QMouseEvent* event)    override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event)        override;

    // ── Вспомогательные ───────────────────────────────────────────
    /// Индекс сепаратора колонки под курсором (0 или 1), или -1
    int  hitTestColumnSeparator(int x) const;
    /// Зажимает m_scrollOffset в допустимый диапазон [0, maxOffset]
    void clampScrollOffset(int totalVirtualRows);
    /// Центрирует вид на спреде — вызывается при первом получении данных
    void centerOnSpread();

    // ── Данные и цвета ─────────────────────────────────────────────
    OrderBookModel  m_model;
    OrderBookColors m_colors;

    // ── Layout ─────────────────────────────────────────────────────
    float m_rowHeight        = 22.0f;
    float m_headerHeight     = 24.0f;
    float m_spreadHeight     = 28.0f;
    /// Все три колонки — абсолютные пиксели, каждая тянется своим разделителем.
    /// После правой границы Total — пустое пространство виджета.
    float m_columnPriceWidth = 80.0f;
    float m_columnQtyWidth   = 70.0f;
    float m_columnTotalWidth = 70.0f;
    bool  m_totalPinned      = false; ///< true после первого ручного drag-а Total

    // ── Скролл ─────────────────────────────────────────────────────
    /// Индекс первой видимой строки единого списка (asks→spread→bids)
    int m_scrollOffset = 0;

private:
    static constexpr int SEPARATOR_HIT_RADIUS = 4; ///< px, зона захвата сепаратора

    enum class DragMode { None, ResizeColumn };

    DragMode m_dragMode          = DragMode::None;
    int      m_dragStartX        = 0;
    int      m_dragColumnIdx     = -1;
    float    m_dragColWidthAtStart = 0.0f;
};
