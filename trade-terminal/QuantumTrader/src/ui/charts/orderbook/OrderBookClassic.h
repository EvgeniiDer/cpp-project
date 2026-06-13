#pragma once
#include"IOrderBook.h"
#include"OrderBookRenderTypes.h"

class OrderBookClassic : public IOrderbook
{
	Q_OBJECT
public:
	explicit OrderBookClassic(QWidget* parent = nullptr);

	void setData(const OrderBookSnapshot& snapshot)override;
	void setDepth(int rows) override;

	const OrderBookSnapshot& snapshot()const override { return m_data; }
	void applyDelta(const OrderBookSnapshot& delta);
protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	void drawHeader(QPainter& p, const QRect& rect);
	void drawAsks(QPainter& p, const QRect& rect);
	void drawBids(QPainter& p, const QRect& rect);
	void drawSpread(QPainter& p, const QRect& rect);
	void drawRow(QPainter& p, const QRectF& rect, const RowContext& ctx);
	void drawVolumeCircle(QPainter& p, const QRectF& cellRect, const VolumeCircle& circle);

	double calcMaxTotal(const std::vector<OrderBookLevel>& levels)const;

	OrderBookSnapshot m_data;
	int   m_depth = 20;
	float m_rowHeight = 22.0f;
	float m_headerH = 24.0f;
	float m_spreadH = 28.0f;

	// Ширины колонок — пересчитываются в resizeEvent
	float m_colPrice = 0.0f;
	float m_colQty = 0.0f;
	float m_colTotal = 0.0f;

	// Цвета
	QColor m_bgColor{ 0x14, 0x16, 0x1a };
	QColor m_askColor{ 0xf6, 0x46, 0x5d };
	QColor m_bidColor{ 0x0e, 0xcb, 0x81 };
	QColor m_askBg{ 0xf6, 0x46, 0x5d, 30 };
	QColor m_bidBg{ 0x0e, 0xcb, 0x81, 30 };
	QColor m_textColor{ 0xd1, 0xd5, 0xdb };
	QColor m_headerColor{ 0x6b, 0x72, 0x80 };
	QColor m_spreadBg{ 0x1f, 0x22, 0x28 };
};
