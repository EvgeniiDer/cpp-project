#pragma once

#include<QString>
#include<cstdint>


struct ChartInterval
{
	enum class Unit
	{
		Tick,
		Volume,
		Range,
		Minute,
		Hour,
		Day,
		Week,
		Month
	};

	Unit unit;
	int count;

	ChartInterval(Unit u = Unit::Minute, int c = 1) : unit(u), count(c)
	{

	}

	int64_t toSeconds()const
	{
		switch (unit)
		{
		case Unit::Minute: 
			return count * 60;
		case Unit::Hour:
			return count * 3600;
		case Unit::Day:
			return count * 86400;
		case Unit::Week:
			return count * 604800;
		default: 
			return 0;
		}
	}
	QString toCacheKey()const
	{
		QString unitStr;
		switch (unit)
		{
		case Unit::Tick:
			unitStr = "Tick";
			break;
		case Unit::Volume:
			unitStr = "Volume";
			break;
		case Unit::Range:
			unitStr = "Range";
			break;
		case Unit::Minute:
			unitStr = "Minute";
			break;
		case Unit::Hour:
			unitStr = "Hour";
			break;
		case Unit::Day:
			unitStr = "Day";
			break;
		case Unit::Week:
			unitStr = "Week";
			break;
		case Unit::Month:
			unitStr = "Month";
			break;

		}
		return QString(QObject::tr("%1_%2").arg(unitStr).arg(count));
	}
	bool operator==(const ChartInterval& other) const
	{
		return unit == other.unit && count == other.count;
	}
	bool operator!=(const ChartInterval& other) const
	{
		return !(*this == other);
	}

};

struct Candle
{
	int64_t timestamp;
	double open;
	double high;
	double low;
	double close;
	double volume;
};

enum class ConnectionState
{
	Disconnected,
	Connecting,
	Connected,
	Error
};




