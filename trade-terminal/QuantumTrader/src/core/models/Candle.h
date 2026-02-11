#pragma once

#include<cstdint>


struct Candle
{
	int64_t timestamp;  // Unix timestamp in milliseconds
	double open;       // Opening price
	double high;       // Highest price
	double low;        // Lowest price
	double close;      // Closing price
	double volume;     // Trading volume
	Candle(int64_t ts, double o, double h, double l, double c, double v)
		: timestamp(ts), open(o), high(h), low(l), close(c), volume(v)
	{
	}

};