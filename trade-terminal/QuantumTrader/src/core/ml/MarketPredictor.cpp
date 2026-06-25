#include"MarketPredictor.h"


MarketPredictor::MarketPredictor(const std::string& modelPath)
	: m_calcer(std::make_unique<ModelCalcerWrapper>(modelPath))
{
	
}

int MarketPredictor::predictNextCandleDirection(const Candle& current, const Candle& previous) const
{
	if (!m_calcer) return -1;
	if (previous.close == 0.0 || previous.volume == 0.0) return -1;

	const float pct_return = static_cast<float>((current.close - previous.close) / previous.close);
	const float hl_ratio = static_cast<float>((current.high / current.low));
	const float volume_change = static_cast<float>((current.volume - previous.volume) / previous.volume);

	const std::vector<float> floatFeatures ={
		static_cast<float>(current.open),
		static_cast<float>(current.high),
		static_cast<float>(current.low),
		static_cast<float>(current.close),
		static_cast<float>(current.volume),
		pct_return,
		hl_ratio,
		volume_change
	};
	const double rawScores = m_calcer->CalcFlat(floatFeatures);
	return(rawScores > 0.0) ? 1 : 0;
}

