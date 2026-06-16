#pragma once
#include "OrderBookDataTypes.h"

/**
 * @brief Модель данных стакана — единственное место где хранятся данные и бизнес-логика.
 *
 * Отвечает ТОЛЬКО за данные и расчёты:
 *  - хранение снапшота (биды/аски)
 *  - мерж дельты (applyDelta)
 *  - вспомогательные расчёты Находит максимальный Total (qty * price) среди уровней — для нормализации баров/кругов
 *  - Находит максимальный объём (qty) среди уровней — для нормализации радиуса кругов
 виджеты и отрисовку.
 */
class OrderBookModel
{
public:
    /// Заменить весь снапшот целиком (например, при первом подключении)
    void setData(const OrderBookSnapshot& snapshot);

    /// Применить инкрементальное обновление от WebSocket-потока
    void applyDelta(const OrderBookSnapshot& delta);

    /// Установить глубину отображения (кол-во строк)
    void setDepth(int rows);
	///return m_snapshot
    const OrderBookSnapshot& snapshot() const { return m_snapshot; }
    int depth() const { return m_depth; }

    /// Находит максимальный Total (qty * price) среди уровней — для нормализации баров/кругов
    static double calcMaxTotal(const std::vector<OrderBookLevel>& levels);

    /// Находит максимальный объём (qty) среди уровней — для нормализации радиуса кругов
    static double calcMaxQty(const std::vector<OrderBookLevel>& levels);

private:
    OrderBookSnapshot m_snapshot;
    int               m_depth = 20;
};
