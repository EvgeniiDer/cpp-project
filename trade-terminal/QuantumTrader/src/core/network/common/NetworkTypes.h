#pragma once
#include<QString>
#include<cstdint>
#include"../src/core/models/Candle.h"
#include<QMetaType>

/** * @brief Интервал (таймфрейм) свечи.
 * @details Состоит из единицы измерения (Unit) и количества этих единиц.
 * Например, (Minute, 5) → 5-минутные свечи.
 * * Доступные единицы измерения (Unit):
 * - Tick: Тик (сделка)
 * - Volume: По объёму (нестандартный)
 * - Range: Range-бары (нестандартный)
 * - Second: Секунды
 * - Minute: Минуты
 * - Hour: Часы
 * - Day: Дни
 * - Week: Недели
 * - Month: Месяцы
 */
struct ChartInterval
{
	/**
		* @brief Единицы измерения интервала.
		*/
	enum class Unit
	{
		/// Тик (сделка)
		Tick,
		/// По объёму (нестандартный)
		Volume,  
		/// Range-бары (нестандартный)
		Range,   
		/// Секунды
		Second,  
		/// Минуты
		Minute,  
		/// Часы
		Hour,
		/// Дни
		Day, 
		/// Недели
		Week,   
		/// Недели
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
/**
 * @brief Тип данных WebSocket-потока.
 * @details Используется WebSocketPool для изоляции потоков по типу:
 * каждый тип получает свой пул воркеров, чтобы тяжёлые стаканы
 * не мешали свечам и наоборот.
 */
enum class StreamType
{
	/// Свечи (таймфреймы 1m, 5m, 1h и т.д.)
	Kline,

	/// Стакан заявок (Order Book Depth)
	OrderBook,

	/// Последние сделки (Public Trades)
	Trades
};

/**
 * @brief Универсальный контекст рыночного запроса.
 *
 * Передаётся между слоями: Chart -> MarketDataManager -> Connector -> Pool.
 * Содержит все параметры для получения исторических данных или подписки.
 *
 * Поля:
 * - chartId: ID графика (для многооконного режима)
 * - exchange: Имя биржи ("Bybit", "Binance")
 * - symbol: Торговая пара ("BTCUSDT")
 * - marketType: Тип рынка ("SPOT", "PERP", "INV_PERP", "OPTION")
 * - interval: Таймфрейм свечей (ChartInterval)
 * - limit: Максимум элементов (дефолт 1000)
 * - endTime: Конечная метка времени в мс (0 = текущее время)
 * - streamType: Тип WebSocket-потока (Kline / OrderBook / Trades)
 */
struct MarketContext
{
	int           chartId = 0;            ///< ID графика (многооконный режим)
	QString       exchange;                  ///< Имя биржи ("Bybit", "Binance")
	QString       symbol;                    ///< Торговая пара ("BTCUSDT")
	QString       marketType;                ///< Тип рынка ("SPOT", "PERP"...)
	ChartInterval interval;                  ///< Таймфрейм свечей
	int           limit = 1000;         ///< Максимум элементов
	qint64        endTime = 0;            ///< Конечная метка времени (мс), 0 = сейчас
	StreamType    streamType = StreamType::Kline; ///< Тип WS-потока
};
/**
 * @brief Инструмент рынка (торговая пара + биржа + тип рынка).
 *  Идентифицирует уникальный торговый инструмент.
 * Используется как ключ для кэша, подписок и сравнения.
 *
 * Поля:
 * - exchange: Имя биржи ("Binance", "Bybit", "OKX").
 * - symbol: Торговая пара ("BTCUSDT", "ETHUSD").
 * - marketType: Тип рынка ("SPOT" — спот, "PERP" — бессрочный фьючерс,
 *   "INV_PERP" — инверсный бессрочный, "OPTION" — опцион).
 */
struct MarketInstrument
{
	QString exchange;
	QString symbol;
	QString marketType;

	bool operator==(const MarketInstrument& o) const
	{
		return exchange == o.exchange && symbol == o.symbol && marketType == o.marketType;
	}

	QString toCachKey()const
	{
		return exchange + "_" + marketType + "_" + symbol;
	}
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
Q_DECLARE_METATYPE(StreamType);
Q_DECLARE_METATYPE(MarketInstrument)



