#pragma once
#include<QString>
#include<cstdint>
#include"../src/core/models/Candle.h"
#include<QMetaType>
struct ChartInterval
{
	enum class Unit
	{
		Tick,
		Volume,
		Range,
		Second,
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
		case Unit::Month:
			return count * 2592000;
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
		case Unit::Second:
			unitStr = "Second";
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
		return QString("%1_%2").arg(unitStr).arg(count);
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
enum class ConnectionState
{
	Disconnected,
	Connecting,
	Connected,
	Error
};
struct MarketContext
{
	int chartId = 0;
	QString exchange;
	QString symbol;
	QString marketType;
	ChartInterval interval;
	int limit = 1000;
	qint64 endTime = 0;
};
// 🎯 Регистрация типа в мета-системе Qt.
// #include нужен компилятору C++, чтобы знать размер структуры в нашем коде.
// Но макрос обязателен для Qt: он учит универсальный контейнер QVariant
// (который скрыто используется в QComboBox, QAction, setProperty)
// выделять память, копировать и безопасно удалять наш кастомный тип данных.
// ТАК он отличаетья тем что он не может создавать сложную структуру типа данных
// и рабоать между разными потокам в отличии от 	qRegisterMetaType<QList<QPair<QString, QString>>>("QList<QPair<QString,QString>>");
Q_DECLARE_METATYPE(ChartInterval);
Q_DECLARE_METATYPE(MarketContext);



