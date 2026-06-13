#include"OrderBookClassic.h"

#include<QPainter>
#include<QResizeEvent>
#include<cmath>
#include<algorithm>

namespace
{
	constexpr float COL_PRICE_RATIO = 0.40f;
	constexpr float COL_QTY_RATIO = 0.30f;
}

OrderBookClassic::OrderBookClassic(QWidget* parent)
	:IOrderbook(parent)
{
	this->setMinimumWidth(220);
	this->setAttribute(Qt::WA_OpaquePaintEvent);
}

void OrderBookClassic::setData(const OrderBookSnapshot& snapshot)
{
	m_data = snapshot;
	update();
}

void OrderBookClassic::setDepth(int rows)
{
	m_depth = std::max(1, rows);
	update();
}

void OrderBookClassic::resizeEvent(QResizeEvent* event)
{
	float w = static_cast<float>(event->size().width());
	m_colPrice = w * COL_PRICE_RATIO;
	m_colQty = w * COL_QTY_RATIO;
	m_colTotal = w - m_colPrice - m_colQty;
	QWidget::resizeEvent(event);
}
void OrderBookClassic::applyDelta(const OrderBookSnapshot& delta)
{
	auto merge = [](std::vector<OrderBookLevel>& book,
		const std::vector<OrderBookLevel>& updates,
		bool descending)
		{
			for (const auto& upd : updates)
			{
				auto it = std::find_if(book.begin(), book.end(),
					[&](const OrderBookLevel& l) { return l.price == upd.price; });
				if (upd.qty == 0.0)
				{
					if (it != book.end()) book.erase(it);
				} else
				{
					if (it != book.end()) it->qty = upd.qty;
					else book.push_back(upd);
				}
			}
			if (descending)
				std::sort(book.begin(), book.end(),
					[](const OrderBookLevel& a, const OrderBookLevel& b) { return a.price > b.price; });
			else
				std::sort(book.begin(), book.end(),
					[](const OrderBookLevel& a, const OrderBookLevel& b) { return a.price < b.price; });
		};

	merge(m_data.bids, delta.bids, true);   // биды: цена по убыванию
	merge(m_data.asks, delta.asks, false);  // аски: цена по возрастанию
	update();
}
//paintEvent 
void OrderBookClassic::paintEvent(QPaintEvent* event)
{
	// Инициализируем отрисовщик на стеке для быстрой и автоматической очистки ресурсов
	QPainter p(this);

	// ОТКЛЮЧАЕМ сглаживание, чтобы текст и прямоугольники объемов были идеально резкими, пиксель-в-пиксель
	p.setRenderHint(QPainter::Antialiasing, false);

	// Полностью закрашиваем фон всего виджета базовым темно-серым цветом
	p.fillRect(0, 0, width(), height(), m_bgColor);

	// Рисуем шапку таблицы (зону с названиями колонок Price, Size, Total)
	drawHeader(p, QRect(0, 0, width(), static_cast<int>(m_headerH)));

	// Считаем чистую высоту экрана, оставшуюся для строк (вычитаем из общей высоты шапку и спред)
	float availH = static_cast<float>(height()) - m_headerH - m_spreadH;

	// Вычисляем, сколько строк Asks и Bids физически влезут в это пространство.
	// Делим на 2.0f, так как свободное место делится поровну между верхним и нижним блоками.
	int visibleRows = std::min(m_depth,
		static_cast<int>(std::floor(availH / 2.0f / m_rowHeight)));

	// Предохранитель: если окно сжали до микроскопических размеров, принудительно рисуем хотя бы 1 строку
	if (visibleRows < 1) visibleRows = 1;

	// Считаем точную высоту (в пикселях) для блока асков
	float asksH = visibleRows * m_rowHeight;
	// Находим координату Y, с которой должна начаться отрисовка разделительной полосы спреда
	float spreadY = m_headerH + asksH;

	// 1. Рисуем Аски (продавцы) — они занимают верхнюю зону, идущую строго под шапкой
	drawAsks(p, QRect(0, static_cast<int>(m_headerH), width(), static_cast<int>(asksH)));

	// 2. Рисуем Спред (информационный блок по центру) — он располагается сразу под асками
	drawSpread(p, QRect(0, static_cast<int>(spreadY), width(), static_cast<int>(m_spreadH)));

	// 3. Рисуем Биды (покупатели) — они занимают нижнюю зону, которая начинается строго под спредом
	drawBids(p, QRect(0, static_cast<int>(spreadY + m_spreadH),
		width(), static_cast<int>(visibleRows * m_rowHeight)));

}
//Шапка
void OrderBookClassic::drawHeader(QPainter& p, const QRect& rect)
{
	p.fillRect(rect, m_spreadBg);
	QFont f = p.font();//Шрифты!!
	f.setPixelSize(11);
	p.setFont(f);
	p.setPen(m_headerColor);
	//размещение текста
	p.drawText(QRectF(4, rect.y(), m_colPrice, rect.height()),
		Qt::AlignVCenter | Qt::AlignLeft, "Price");
	p.drawText(QRectF(m_colPrice, rect.y(), m_colQty, rect.height()),
		Qt::AlignVCenter | Qt::AlignRight, "Size");
	p.drawText(QRectF(m_colPrice + m_colQty, rect.y(), m_colTotal - 4, rect.height()),
		Qt::AlignVCenter | Qt::AlignRight, "Total");
}
//asks
void OrderBookClassic::drawAsks(QPainter& p, const QRect& rect)
{
	// Получаем ссылку на вектор асков (продавцов) из снапшота
	const std::vector<OrderBookLevel> asks = m_data.asks;
	if (asks.empty()) return;

	//    Вычисляем 'count' — сколько строк физически влезет в rect, 
	//    но не больше, чем у нас есть реальных элементов в векторе asks
	int count = std::min(static_cast<int>(rect.height() / m_rowHeight), static_cast<int>(asks.size()));

	// Находим самый большой Total среди строк, чтобы правильно масштабировать цветные бары
	double maxTotal = calcMaxTotal(asks);

	for (int i = 0; i < count; ++i)
	{
		// Cамый лучший Ask (asks[0]) оказался в самом низу, прямо над спредом.
		// Поэтому при i = 0 (первая итерация, нижняя строка) мы берем самый дальний элемент: count - 1.
		int idx = count - 1 - i;//худший ask сверху лучший с низу

		// Cтартуем от НИЖНЕЙ границы зоны асков (rect.bottom()) и ВЫЧИТАЕМ пиксели,
		// чтобы каждая следующая строка поднималась всё ВЫШЕ и ВЫШЕ к потолку.
		float rowY = rect.bottom() - (i + 1) * m_rowHeight;
		
		// Создаем структуру для будущего круга объема. 
		// Передаем объем текущей строки, а в качестве макса (maxQty) берем asks[0].qty.
		// Так как вектор asks отсортирован, у лучшего аска (asks[0]) обычно самый высокий приоритет и объем.
		// true означает, что круг будет окрашен в цвет Асков (красный).
		VolumeCircle vc{
			asks[idx].qty,
			(count > 0 ? asks[0].qty : 0.0),
			true
		};
		// Упаковываем все данные в один контен 
		RowContext ctx
		{
			asks[idx],
			maxTotal,
			true,
			vc
		};
		// Вызываем метод отрисовки одной конкретной строки на холсте
		drawRow(p, QRect(rect.left(), rowY, rect.width(), m_rowHeight), ctx);
	}
}
//Bids
void OrderBookClassic::drawBids(QPainter& p, const QRect& rect)
{
	const std::vector<OrderBookLevel>& bids = m_data.bids;
	if (bids.empty()) return;

	int    count = std::min(static_cast<int>(rect.height() / m_rowHeight),
		static_cast<int>(bids.size()));
	double maxTotal = calcMaxTotal(bids);

	for (int i = 0; i < count; ++i)
	{
		// Здесь переворачивать индекс НЕ НАДО! 
		// Лучший Bid (bids[0]) должен быть в самом верху зоны бидов — прямо под спредом.
		// Поэтому при i = 0 мы рисуем bids[0], при i = 1 рисуем bids[1] и т.д.
		// Переменная `i` напрямую используется как индекс.

		// Магия координат Y!
		// Стартуем от ВЕРХНЕЙ границы зоны бидов (rect.top()) и ПРИБАВЛЯЕМ пиксели,
		// чтобы каждая следующая строка уходила всё НИЖЕ и НИЖЕ к полу.
		float  rowY = rect.top() + i * m_rowHeight;

		VolumeCircle vc
		{
			bids[i].qty,
			(count > 0 ? bids[0].qty : 0.0),
			false
		};
		RowContext ctx
		{
			bids[i],
			maxTotal,
			false,
			vc 
		};
		drawRow(p, QRectF(rect.left(), rowY, rect.width(), m_rowHeight), ctx);
	}
}

// Спред
void OrderBookClassic::drawSpread(QPainter& p, const QRect& rect)
{
	p.fillRect(rect, m_spreadBg);
	p.setPen(QColor(0x2d, 0x31, 0x3a));
	p.drawLine(rect.left(), rect.top(), rect.right(), rect.top());
	p.drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

	if (m_data.asks.empty() || m_data.bids.empty()) return;

	double bestAsk = m_data.asks.front().price;
	double bestBid = m_data.bids.front().price;
	double spread = bestAsk - bestBid;
	double midPrice = (bestAsk + bestBid) / 2.0;
	double spreadPct = (midPrice > 0.0) ? (spread / midPrice * 100.0) : 0.0;

	QFont f = p.font(); f.setPixelSize(12); f.setBold(true); p.setFont(f);
	p.setPen(m_textColor);
	p.drawText(QRectF(0, rect.top(), width() * 0.55f, rect.height()),
		Qt::AlignVCenter | Qt::AlignRight, formatPrice(midPrice));

	f.setBold(false); f.setPixelSize(11); p.setFont(f);
	p.setPen(m_headerColor);
	p.drawText(QRectF(width() * 0.55f, rect.top(), width() * 0.45f, rect.height()),
		Qt::AlignVCenter | Qt::AlignLeft,
		QString("  %1 (%2%)")
		.arg(formatTotal(spread))
		.arg(QString::number(spreadPct, 'f', 3)));
}

//Круги Обьема задел на будущее
void OrderBookClassic::drawVolumeCircle(QPainter& p, const QRectF& cellRect,
	const VolumeCircle& circle)
{
	float maxR = static_cast<float>(
		std::min(cellRect.width(), cellRect.height()) / 2.0 * 0.85);
	float r = maxR * circle.normalizedRadius();
	if (r < 1.0f) return;

	QColor color = circle.isAsk
		? QColor(0xf6, 0x46, 0x5d, 180)
		: QColor(0x0e, 0xcb, 0x81, 180);
	p.setPen(Qt::NoPen);
	p.setBrush(color);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.drawEllipse(cellRect.center(),
		static_cast<double>(r),
		static_cast<double>(r));
	p.setRenderHint(QPainter::Antialiasing, false);
	p.setBrush(Qt::NoBrush);
}
double OrderBookClassic::calcMaxTotal(const std::vector<OrderBookLevel>& levels) const
{
	double mx = 0.0;
	for (const auto& lv : levels)
	{
		double t = lv.qty * lv.price;
		if (t > mx) mx = t;
	}
	return mx;
}
void OrderBookClassic::drawRow(QPainter& p, const QRectF& rect, const RowContext& ctx)
{
	double total = ctx.level.qty * ctx.level.price;
	double ratio = (ctx.maxTotal > 0.0) ? std::min(1.0, total / ctx.maxTotal) : 0.0;
	float  barW = static_cast<float>(rect.width() * ratio);

	// Volume bar
	p.fillRect(QRectF(rect.right() - barW, rect.top(), barW, rect.height()),
		ctx.isAsk ? m_askBg : m_bidBg);

	QFont f = p.font(); f.setPixelSize(12); p.setFont(f);

	// Цена
	p.setPen(ctx.isAsk ? m_askColor : m_bidColor);
	p.drawText(QRectF(rect.left() + 4, rect.top(), m_colPrice - 4, rect.height()),
		Qt::AlignVCenter | Qt::AlignLeft,
		formatPrice(ctx.level.price));

	// Объём
	p.setPen(m_textColor);
	p.drawText(QRectF(rect.left() + m_colPrice, rect.top(), m_colQty - 4, rect.height()),
		Qt::AlignVCenter | Qt::AlignRight,
		formatQty(ctx.level.qty));

	// Сумма
	p.drawText(QRectF(rect.left() + m_colPrice + m_colQty, rect.top(), m_colTotal - 4, rect.height()),
		Qt::AlignVCenter | Qt::AlignRight,
		formatTotal(total));
}
