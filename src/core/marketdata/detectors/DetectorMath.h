#pragma once
#include"../types/OrderBookDataTypes.h"

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
}
