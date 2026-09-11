#pragma once
#include<QtGlobal>

struct OrderBookFeatureRow
{
	qint64 timestamp = 0;
	double bestBid = 0.0;
	double bestAsk = 0.0;
	double spread = 0.0;
	float imbalanceL1 = 0.0f;
	float imbalanceSma3 = 0.0f;
	float bidSpoofDrop = 0.0f;
	float askSpoofDrop = 0.0f;
	float bidFakeRatio = 0.0f;
	float askFakeRatio = 0.0f;
	float volumeDepthBids = 0.0f;
	float volumeDepthAsks = 0.0f;
	double midPrice = 0.0;

	int bidIcebergRefillCount = 0;
	int askIcebergRefillCount = 0;
	float bidIcebergRefillQty = 0.0f;
	float askIcebergRefillQty = 0.0f;

	bool isValid = false;
};