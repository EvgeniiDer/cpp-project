#pragma once

#include<vector>
#include<string>
#include<memory>
#include"../../external/catboost/include/wrapped_calcer.h"
#include "core/models/Candle.h"

class MarketPredictor
{
public:
	explicit MarketPredictor(const std::string& modelPath);
	MarketPredictor(const MarketPredictor&) = delete;
	MarketPredictor& operator=(const MarketPredictor&) = delete;
	MarketPredictor(MarketPredictor&&) = default;
	MarketPredictor& operator=(MarketPredictor&&) = default;
	~MarketPredictor();

	int predictNextCandleDirection(const Candle& current, const Candle& previous)const;
private:
	std::unique_ptr<ModelCalcerWrapper> m_calcer;
};
