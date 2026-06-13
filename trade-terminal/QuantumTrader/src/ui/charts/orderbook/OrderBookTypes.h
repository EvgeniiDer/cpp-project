#pragma once
#include<vector>
#include<QString>


struct OrderBookLevel
{
	double price = 0.0;
	double qty = 0.0;
};

struct OrderBookSnapshot
{
	QString exchange;
	QString symbol;
	std::vector<OrderBookLevel> asks;
	std::vector<OrderBookLevel> bids;
};