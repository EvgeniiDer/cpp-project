#pragma once
#include<QWidget>
#include"OrderBookTypes.h"


class IOrderbook : public QWidget
{
	Q_OBJECT
public:
	explicit IOrderbook(QWidget* parent = nullptr) : QWidget(parent){}

	virtual ~IOrderbook()override = default;

	virtual void setData(const OrderBookSnapshot& snapshot) = 0;
	virtual void setDepth(int rows) = 0;
	virtual const OrderBookSnapshot& snapshot() const = 0;
protected:
	static QString formatPrice(double price);
	static QString formatQty(double qty);
	static QString formatTotal(double total);
};