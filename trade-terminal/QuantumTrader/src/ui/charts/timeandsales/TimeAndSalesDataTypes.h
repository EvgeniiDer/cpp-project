#pragma once
#include <QString>

/**
 * @brief Один исполненный тик сделки (Time & Sales).
 *
 * @var time    Время сделки в формате "HH:mm:ss.zzz"
 * @var price   Цена исполнения
 * @var qty     Объём сделки
 * @var isBuy   true = покупка (зелёный), false = продажа (красный)
 */
struct TradeTick
{
    QString time;
    double  price = 0.0;
    double  qty   = 0.0;
    bool    isBuy = false;
};
