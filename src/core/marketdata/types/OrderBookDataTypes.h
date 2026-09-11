#pragma once
#include<vector>
#include<QString>

/**
 * @brief Один исполненный тик сделки (Time & Sales).
 *
 * @var price   Цена исполнения
 * @var qty     Объём сделки
 * @var isBuy   true = покупка (зелёный), false = продажа (красный)
 */
struct TradeTick
{
	qint64 timestampMs = 0;
	double price = 0.0;
	double qty = 0.0;
	bool isBuy = false;
};
/**
 * @brief Уровень стакана (один уровень цен).
 *
 *  Содержит два поля:
 * - price: цена (обычно в котируемой валюте, например USDT)
 * - qty:   объём (количество базового актива, например BTC)
 *
 * @var price Цена уровня.
 * @var qty   Объём заявок по этой цене.
 */
struct OrderBookLevel
{
	double price = 0.0;
	double qty = 0.0;
};
/**
 * @brief Снимок стакана заявок (Order Book) на определённый момент времени.
 *
 * @details 
 *  полное состояние биржевого стакана для одного инструмента:
 * - список заявок на продажу (аски, asks)
 * - список заявок на покупку (биды, bids)
 * - std::vector<OrderBookLevel> asks;
 * - std::vector<OrderBookLevel> bids;
 * @see OrderBookLevel — отдельный уровень стакана (цена + объём).
 */
struct OrderBookSnapshot
{
	QString exchange;
	QString symbol;
	qint64 timestamp = 0;

	std::vector<OrderBookLevel> asks;///< Аски (продажа): вектор уровней, каждый с price и volume
	std::vector<OrderBookLevel> bids;///< Биды (покупка): вектор уровней, каждый с price и volume
};
