#pragma once
#include"../types/OrderBookDataTypes.h"
#include<cmath>
#include<vector>


namespace DetectorMath
{
	constexpr double kPriceEpsilon = 1e-9;

	inline bool priceEqual(double a, double b)
	{
		return std::fabs(a - b) < kPriceEpsilon;
	}

	inline double sumTradedQtyNearPrice(const std::vector<TradeTick>& trades, double price, double tickTolerance)
	{
		double sum = 0.0;
		for (const TradeTick& t : trades)
		{
			if (std::fabs(t.price - price) <= tickTolerance)
			{
				sum += t.qty;
			}
		}
		return sum;
	}
	inline double sumTradedQtyAtPriceByAggressor(const std::vector<TradeTick>& trades, double price, double priceTolerance, bool aggressorIsBuy)
	{
		double sum = 0.0;
		for (const TradeTick& t : trades)
		{
			if (t.isBuy == aggressorIsBuy && std::fabs(t.price - price) <= priceTolerance)
			{
				sum += t.qty;
			}
		}
	return sum;
	}
}
