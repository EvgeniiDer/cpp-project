#include "OrderBookBase.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCursor>
#include <cmath>
#include <algorithm>

namespace
{
	/**
	 * @brief Форматирует число с сокращением тысяч/миллионов.
	 * @param v число
	 * @param decimals кол-во знаков после запятой для чисел < 1000
	 * @param Примеры:
	 * - fmtDouble(1234.56, 2) → "1.23K"
	 * - fmtDouble(1234567.89, 2) → "1.23M"
	 * - fmtDouble(567.89, 2) → "567.89"
	 * @return строка вида "1.23K", "4.56M" или "123.45"
	 */
	QString fmtDouble(double v, int decimals)
	{
		if (v >= 1'000'000.0)
			{
				return QString::number(v / 1'000'000.0, 'f', 2) + "M";
			}
		if (v >= 1'000.0)
			{
				return QString::number(v / 1'000.0, 'f', 2) + "K";
			}
		return QString::number(v, 'f', decimals);
	}
}

OrderBookBase::OrderBookBase(QWidget* parent)
    : QWidget(parent)
{}
/**
 * @brief Заменить текущий стакан новым полным снимком.
 * @param snapshot Новый снапшот стакана (полный набор асков и бидов).
 *
 * @note После обновления данных автоматически вызывается `update()`,
 *       что приводит к перерисовке виджета.
 *
 * @see applyDelta() – для инкрементальных обновлений.
 */
void OrderBookBase::setData(const OrderBookSnapshot& snapshot)
{
    const bool firstData = m_model.snapshot().asks.empty();
    m_model.setData(snapshot);
    if (firstData && !snapshot.asks.empty())
        centerOnSpread(); // при первом получении данных — центрируем на спреде
    update();
}
/**
 * @brief Применить инкрементальное обновление (дельту) к текущему стакану.
 * @param delta Снапшот, содержащий только изменившиеся уровни (аски и биды).
 *
 * @details Выполняет слияние переданной дельты с текущим состоянием модели:
 *          - Если уровень с такой ценой уже существует – обновляет его объём
 *            (при объёме = 0 уровень удаляется).
 *          - Если уровень отсутствует – добавляется новый.
 *
 *          После слияния уровни сортируются:
 *          - Аски (продажа) — по возрастанию цены (от лучшей к худшей).
 *          - Биды (покупка) — по убыванию цены (от лучшей к худшей).
 *
 *          Это гарантирует, что первый элемент в каждом векторе всегда
 *          соответствует лучшей цене (минимальный аск, максимальный бид).
 *
 *          Это основной метод для обработки WebSocket-обновлений в реальном времени.
 *
 * @note После применения дельты автоматически вызывается `update()`,
 *       чтобы виджет отобразил актуальное состояние.
 *
 * @see setData() – для полной замены стакана.
 */void OrderBookBase::applyDelta(const OrderBookSnapshot& delta)
{
    m_model.applyDelta(delta);
    update();
}
 /**
  * @brief Установить глубину отображения стакана (количество видимых уровней).
  * @param rows Количество строк (уровней) для отображения как для асков, так и для бидов.
  *
  * @details Глубина определяет, сколько лучших уровней (с наилучшими ценами)
  *          будет показано в виджете. Например, при depth = 10 отображаются
  *          10 лучших асков и 10 лучших бидов.
  *
  *          Сами данные в модели при этом не теряются — полный снапшот стакана
  *          остаётся в памяти, но отрисовывается только указанное количество уровней.
  *
  * @note Изменение глубины не влияет на получение данных от биржи — это только
  *       настройка отображения. После изменения вызывается `update()` для
  *       перерисовки виджета с новой глубиной.
  *
  * @see depth() – получить текущую глубину.
  */
void OrderBookBase::setDepth(int rows)
{
    m_model.setDepth(rows);
    update();
}
/**
 * @brief Форматирует цену с адаптивной точностью в зависимости от её величины.
 *
 * @details Метод выбирает количество знаков после запятой:
 *          - Цена >= 10 000 → 1 знак (BTC, крупные криптовалюты)
 *          - Цена >= 100   → 2 знака (ETH, средние активы)
 *          - Цена >= 1     → 4 знака (ADA, XRP)
 *          - Цена < 1      → 6 знаков (DOGE, SHIB)
 *          Суффиксы K/M не добавляются — цена показывается полностью.
 *          <br>
 *          Примеры:
 *          <br>
 *          - formatPrice(65000.123) → "65000.1"
 *          - formatPrice(3500.567)  → "3500.57"
 *          - formatPrice(2.34567)   → "2.3457"
 *          - formatPrice(0.000123)  → "0.000123"
 *
 * @param price Цена (положительное число).
 * @return Отформатированная строка с ценой.
 */
QString OrderBookBase::formatPriceToString(double price)
{
    if (price >= 10000.0)
    {
        return QString::number(price, 'f', 1);
    }
    if (price >= 100.0)
    {
        return QString::number(price, 'f', 2);
    }
    if (price >= 1.0)
    {
        return QString::number(price, 'f', 4);
    }
    return QString::number(price, 'f', 6);
}

QString OrderBookBase::formatQty(double levelVolume)
{
	return fmtDouble(levelVolume,   4);
}
QString OrderBookBase::formatTotal(double PriceMultipliedByVolumeAtOneLevel)
{
	return fmtDouble(PriceMultipliedByVolumeAtOneLevel, 0);
}

void OrderBookBase::drawHeader(QPainter& p, const QRect& rect)
{
    p.fillRect(rect, m_colors.spreadBg);

    QFont f = p.font();
    f.setPixelSize(11);
    p.setFont(f);
    p.setPen(m_colors.headerColor);

    p.drawText(QRectF(4, rect.y(), m_columnPriceWidth - 4, rect.height()),Qt::AlignVCenter | Qt::AlignLeft,  "Price");
    p.drawText(QRectF(m_columnPriceWidth, rect.y(), m_columnQtyWidth - 4, rect.height()),Qt::AlignVCenter | Qt::AlignRight, "Size");
    p.drawText(QRectF(m_columnPriceWidth + m_columnQtyWidth, rect.y(), m_columnTotalWidth - 4, rect.height()),Qt::AlignVCenter | Qt::AlignRight, "Total");

    // Вертикальные разделители — яркие полоски для drag-а Для изменения цвета менять p.setPen(QPen(QColor.....));
    p.setPen(QPen(QColor(0xff, 0xff, 0xff), 5));
    auto drawSep = [&](float x) 
	{
        int xi = static_cast<int>(x);// приведение типа приводит аргумент к типу int
        p.drawLine(xi, rect.top() + 2, xi, rect.bottom() - 2);
    };
    drawSep(m_columnPriceWidth); 
    drawSep(m_columnPriceWidth + m_columnQtyWidth);
    if (m_columnTotalWidth > 0.0f) //набудущее для отключении колонки
    {
        drawSep(m_columnPriceWidth + m_columnQtyWidth + m_columnTotalWidth);
    }
}

void OrderBookBase::drawSpread(QPainter& p, const QRect& rect)
{
    p.fillRect(rect, m_colors.spreadBg);
    p.setPen(QColor(0x2d, 0x31, 0x3a));
    p.drawLine(rect.left(), rect.top(),    rect.right(), rect.top());
    p.drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

    const OrderBookSnapshot& snap = m_model.snapshot();
    if (snap.asks.empty() || snap.bids.empty())
        return;

    double bestAsk   = snap.asks.front().price;// лучшая цена по биду
    double bestBid   = snap.bids.front().price;// лучшая цена по аскау
    double spread    = bestAsk - bestBid;// spread
    double midPrice  = (bestAsk + bestBid) / 2.0;
    double spreadPct = (midPrice > 0.0) ? (spread / midPrice * 100.0) : 0.0; // процентное отношение спреда

    QFont f = p.font();
    f.setPixelSize(12);
	f.setBold(true);
    p.setFont(f);
    p.setPen(m_colors.textColor);
    p.drawText(QRectF(0, rect.top(), this->width() * 0.55f, rect.height()),Qt::AlignVCenter | Qt::AlignRight, formatPriceToString(midPrice));

    f.setBold(false);
	f.setPixelSize(11);
    p.setFont(f);
    p.setPen(m_colors.headerColor);
    p.drawText(QRectF(width() * 0.55f, rect.top(), this->width() * 0.45f, rect.height()),Qt::AlignVCenter | Qt::AlignLeft,QString("  %1 (%2%)")
                   .arg(formatTotal(spread))
                   .arg(QString::number(spreadPct, 'f', 3)));
}

void OrderBookBase::drawRow(QPainter& p, const QRectF& rect, const RowContext& ctx)
{
    // Фоновый бар объёма — от правого края Total, не выходит за его границу
    double total      = ctx.level.qty * ctx.level.price;
    double ratio      = (ctx.maxTotal > 0.0) ? std::min(1.0, total / ctx.maxTotal) : 0.0;
    float  contentW   = m_columnPriceWidth + m_columnQtyWidth + m_columnTotalWidth;
    float  barW       = static_cast<float>(contentW * ratio);
    float  totalRight = rect.left() + contentW;

    p.fillRect(QRectF(totalRight - barW, rect.top(), barW, rect.height()),
               ctx.isAsk ? m_colors.askBg : m_colors.bidBg);

    QFont f = p.font();
    f.setPixelSize(12);
    p.setFont(f);

    // Цена
    p.setPen(ctx.isAsk ? m_colors.askColor :m_colors.bidColor);
    p.drawText(QRectF(rect.left() + 4,rect.top(), m_columnPriceWidth - 4,  rect.height()),Qt::AlignVCenter | Qt::AlignLeft, formatPriceToString(ctx.level.price));

    // Объём
    p.setPen(m_colors.textColor);
    p.drawText(QRectF(rect.left() + m_columnPriceWidth,rect.top(), m_columnQtyWidth - 4,   rect.height()),Qt::AlignVCenter | Qt::AlignRight, formatQty(ctx.level.qty));

    // Сумма
    p.drawText(QRectF(rect.left() + m_columnPriceWidth + m_columnQtyWidth,  rect.top(), m_columnTotalWidth - 4, rect.height()),Qt::AlignVCenter | Qt::AlignRight, formatTotal(total));
}

/** НЕИСПОЛЬУЕТЬСЯ
 * @brief Рисует круг объёма в заданном центре с заданным радиусом.
 *
 * @param center     центр круга в координатах виджета
 * @param radius     радиус в пикселях (уже вычисленный снаружи)
 * @param qty        объём для подписи внутри круга
 * @param isAsk      true = красный (аск), false = зелёный (бид)
 * @param showLabel  рисовать ли текст объёма внутри
 */
void OrderBookBase::drawVolumeCircle(QPainter& p, const QPointF& center, float radius,
                                   double qty, bool isAsk, bool showLabel)
{
    if (radius < 1.0f) return;

    QColor fillColor = isAsk
        ? QColor(0xf6, 0x46, 0x5d, 200)
        : QColor(0x0e, 0xcb, 0x81, 200);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(fillColor);
    p.drawEllipse(center, static_cast<double>(radius), static_cast<double>(radius));

    if (showLabel && radius >= 10.0f)
    {
        QFont f = p.font();
        f.setPixelSize(std::max(8, static_cast<int>(radius * 0.55f)));
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);

        QRectF labelRect(center.x() - radius, center.y() - radius,radius * 2.0,radius * 2.0);
        p.drawText(labelRect, Qt::AlignCenter, formatQty(qty));
    }

    p.setRenderHint(QPainter::Antialiasing, false);
    p.setBrush(Qt::NoBrush);
}

/**
 * @brief Определяет, находится ли координата X мыши рядом с одним из разделителей колонок.
 *
 * @param x Горизонтальная координата курсора (в пикселях, относительно виджета).
 * @return int Номер разделителя, на который наведён курсор:
 *         - 0 — разделитель между колонками Price и Size,
 *         - 1 — между Size и Total,
 *         - 2 — правая граница колонки Total (если она включена),
 *         - -1 — курсор далеко от всех разделителей.
 *
 * @details Функция проверяет расстояние от x до каждого разделителя.
 *          Если расстояние не превышает SEPARATOR_HIT_RADIUS (4 пикселя),
 *          то считается, что курсор «захватил» этот разделитель.
 *
 *          Перед проверками убеждаемся, что колонка Price имеет положительную ширину
 *          (иначе виджет ещё не инициализирован, и проверять бессмысленно).
 *
 *          Порядок проверок важен:
 *          - Сначала проверяется разделитель Price|Size (0),
 *          - затем Size|Total (1),
 *          - затем правая граница Total (2) — только если колонка Total существует.
 *
 *          Если курсор попадает в зону нескольких разделителей (например, если они
 *          оказались рядом из‑за малой ширины), возвращается первый совпавший (приоритет слева направо).
 */
int OrderBookBase::hitTestColumnSeparator(int x) const
{
    if (m_columnPriceWidth <= 0.0f) return -1;
    auto near = [&](float sepX) 
	{
        return std::abs(x - static_cast<int>(sepX)) <= SEPARATOR_HIT_RADIUS;
    };
    if (near(m_columnPriceWidth))
        return 0;
    if (near(m_columnPriceWidth + m_columnQtyWidth))                      
        return 1;
    if (near(m_columnPriceWidth + m_columnQtyWidth + m_columnTotalWidth)) 
        return 2;
    return -1;
}

/**
 * @brief Зажимает offset в [0, totalVirtualRows - 1].
 * @param totalVirtualRows — полное число строк единого списка (asks + 1 spread + bids)
 */
void OrderBookBase::clampScrollOffset(int totalVirtualRows)
{
    m_scrollOffset = std::clamp(m_scrollOffset, 0, std::max(0, totalVirtualRows - 1));
}

/**
 * @brief Устанавливает m_scrollOffset так, чтобы спред оказался
 *        примерно в центре видимой области.
 *
 * Вызывается из setData() при первом получении данных.
 * Формула: смотрим сколько строк влезает в окно, затем
 *   offset = (nAsks) - (visibleRows / 2)
 * т.е. лучший аск оказывается примерно посередине экрана.
 */
void OrderBookBase::centerOnSpread()
{
    const OrderBookSnapshot& snap = m_model.snapshot();
    int nAsks = static_cast<int>(snap.asks.size());
    if (nAsks == 0)
    {
	    m_scrollOffset = 0; return;
    }
    float availH     = static_cast<float>(height()) - m_headerHeight;
    int   visibleRows = std::max(1, static_cast<int>(availH / m_rowHeight));
    m_scrollOffset   = std::max(0, nAsks - visibleRows / 2);
}

void OrderBookBase::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
	    QWidget::mousePressEvent(event); 
    	return;
    }

    const int x = static_cast<int>(event->position().x());
    int sepIdx = hitTestColumnSeparator(x);
    if (sepIdx >= 0)
    {
        m_dragMode = DragMode::ResizeColumn;
        m_dragColumnIdx = sepIdx;
        m_dragStartX = x;
        m_dragColWidthAtStart = (sepIdx == 0) ? m_columnPriceWidth : (sepIdx == 1) ? m_columnQtyWidth : m_columnTotalWidth;
        setCursor(Qt::SizeHorCursor);
    }
    event->accept();
}

void OrderBookBase::mouseMoveEvent(QMouseEvent* event)
{
    const int x = static_cast<int>(event->position().x());

    if (m_dragMode == DragMode::None)
    {
        setCursor(hitTestColumnSeparator(x) >= 0 ? Qt::SizeHorCursor : Qt::ArrowCursor);
        return;
    }

    if (m_dragMode == DragMode::ResizeColumn)
    {
        float delta  = static_cast<float>(x - m_dragStartX);
        float totalW = static_cast<float>(width());
        constexpr float MIN_COL = 5.0f;

        if (m_dragColumnIdx == 0)
        {
            // Граница Price | Size — Price меняется, Size и Total не трогаем
            float maxPrice = totalW - m_columnQtyWidth - m_columnTotalWidth - MIN_COL;
            m_columnPriceWidth = std::clamp(m_dragColWidthAtStart + delta, MIN_COL, maxPrice);
        }
        else if (m_dragColumnIdx == 1)
        {
            // Граница Size | Total — Size меняется, Price и Total не трогаем
            float maxQty = totalW - m_columnPriceWidth - m_columnTotalWidth - MIN_COL;
            m_columnQtyWidth = std::clamp(m_dragColWidthAtStart + delta, MIN_COL, maxQty);
        }
        else
        {
            // Граница Total | пустое пространство — Total меняется, Price и Size не трогаем
            float maxTotal = totalW - m_columnPriceWidth - m_columnQtyWidth;
            m_columnTotalWidth = std::clamp(m_dragColWidthAtStart + delta, MIN_COL, maxTotal);
            m_totalPinned = true;
        }
        update();
    }
    event->accept();
}

void OrderBookBase::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragMode = DragMode::None;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
    else { QWidget::mouseReleaseEvent(event); }
}

/**
 * @brief Колёсико мыши — скролл по единому списку asks→spread→bids.
 * Один тик = 3 строки (как в любом обычном списке).
 * Shift + колёсико = 1 строка (точная подстройка).
 */
void OrderBookBase::wheelEvent(QWheelEvent* event)
{
    const auto& snap  = m_model.snapshot();
    int total = static_cast<int>(snap.asks.size()) + 1 + static_cast<int>(snap.bids.size());

    int steps = -event->angleDelta().y() / 120;
    steps *= (event->modifiers() & Qt::ShiftModifier) ? 1 : 3;

    m_scrollOffset += steps;
    clampScrollOffset(total);
    update();
    event->accept();
}
