#pragma once


#include<vector>
#include<random>
#include<algorithm>
#include"../core/models/Candle.h"


namespace MockDataGenerator
{
	inline std::vector<Candle> generate1MinCandles(int count, double startPrice = 100.0, int64_t startTimestamp = 1672531200000)
	{
		std::vector<Candle> candles;
		candles.reserve(count);

		double currentPrice = startPrice;
		int64_t currentTs = startTimestamp;
		std::random_device rd;
		std::mt19937 gen(rd());

		std::normal_distribution<double> priceDist(0.0, 0.5);

		std::uniform_real_distribution<double> wickDist(0.0, 1.0);
		std::uniform_real_distribution<double> volDist(100.0, 5000.0);

		for (int i = 0; i < count; ++i)
		{
			double open = currentPrice;
			
			double close = open + priceDist(gen);
			double maxBody = std::max(open, close);
			double minBody = std::min(open, close);
			
			double high = maxBody + wickDist(gen);
			double low = minBody - wickDist(gen);

			double volume = volDist(gen);

			candles.emplace_back(currentTs, open, high, low, close, volume);

			currentPrice = close;
			currentTs += 60000;

		}
		return candles;
	}
	inline std::vector<Candle> aggregateCandles(const std::vector<Candle>& baseCandles, int targetMinutes)
	{
		std::vector<Candle> result;
		if (baseCandles.empty() || targetMinutes <= 1)
			return baseCandles;

		result.reserve(baseCandles.size() / targetMinutes + 1);
		for (size_t i = 0; i < baseCandles.size(); i += targetMinutes)
		{
			size_t endIndex = std::min(i + targetMinutes, baseCandles.size());
			const Candle& firstCandle = baseCandles[i];
			int64_t ts = firstCandle.timestamp;

			double open = firstCandle.open;
			double high = firstCandle.high;
			double low = firstCandle.low;
			double volume = 0;
			
			for (size_t j = i; j < endIndex; ++j)
			{
				high = std::max(high, baseCandles[j].high);
				low = std::min(low, baseCandles[j].low);
				volume += baseCandles[j].volume;
			}

			double close = baseCandles[endIndex - 1].close;

			result.emplace_back(ts, open, high, low, close, volume);
		}
		return result;
	}
	
}