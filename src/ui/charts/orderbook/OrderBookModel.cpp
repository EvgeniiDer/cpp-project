#include "OrderBookModel.h"
#include <algorithm>

void OrderBookModel::setData(const OrderBookSnapshot& snapshot)
{
    m_snapshot = snapshot;
}
/** РАЗОБРАНО !!)))
 * @param delta Содержит только изменения (дельту), а не полный стакан.
 *
 * Лямбда merge принимает:
 *   book        - текущий вектор бидов или асков (изменяемый)
 *   updatesDelta - вектор изменений (дельта)
 *   descending   - направление сортировки (true для бидов, false для асков)
 *
 * Алгоритм:
 *   1. Для каждого уровня из дельты ищем в book такой же по цене.
 *      Поиск выполняется std::find_if с предикатом-лямбдой:
 *        [&](const OrderBookLevel& l) { return l.price == updDelta.price; }
 *      Этот предикат возвращает true, когда цены совпадают.
 *   2. std::find_if возвращает итератор:
 *        - если нашли — итератор указывает на найденный элемент
 *        - если нет — итератор равен book.end()
 *   3. Если updDelta.qty == 0.0 — удаляем элемент (если он существует) 
 *      Иначе — обновляем qty (если элемент есть) или добавляем новый (push_back)
 *   4. После обработки всех изменений сортируем book в соответствии с descending
 */
void OrderBookModel::applyDelta(const OrderBookSnapshot& delta)
{
    // Лямбда мержит один вектор (bids или asks) по правилам биржевого протокола:
    //  qty == 0  → удалить уровень
    //  qty  > 0  → обновить если есть, иначе добавить
    //  после мержа — пересортировать
    auto merge = [](std::vector<OrderBookLevel>& book,const std::vector<OrderBookLevel>& updatesDelta, bool descending)
    {
        for (const OrderBookLevel& updDelta : updatesDelta)
        {
            std::vector<OrderBookLevel>::iterator it = std::find_if(book.begin(), book.end(),[&](const OrderBookLevel& l)//предикат который ссылаеться на элемент по которому идет поиск 
                {
	                return l.price == updDelta.price;
                });

            if (updDelta.qty == 0.0)
            {
                if (it != book.end())
                {
                    book.erase(it);
                }
            }
            else
            {
                if (it != book.end())
                {
	                
                    it->qty = updDelta.qty;
                }
                else                 
                	book.push_back(updDelta);
            }
        }

        if (descending)
            std::sort(book.begin(), book.end(),
                [](const OrderBookLevel& a, const OrderBookLevel& b)
                { return a.price > b.price; });
        else
            std::sort(book.begin(), book.end(),
                [](const OrderBookLevel& a, const OrderBookLevel& b)
                { return a.price < b.price; });
    };

    merge(m_snapshot.bids, delta.bids, true);   // биды: лучший (высокий) сверху
    merge(m_snapshot.asks, delta.asks, false);  // аски: лучший (низкий) сверху
}
/**
 * @brief Устанавливает глубину стакана (количество отображаемых уровней).
 *
 * Глубина определяет, сколько лучших уровней бидов и асков будет храниться
 * и отображаться. Например, depth = 10 → показываем только первые 10 строк
 * с каждой стороны. Остальные уровни игнорируются.
 *
 * @param rows Желаемое количество уровней. Если передадут 0 или отрицательное,
 *             устанавливается минимум 1, чтобы стакан не был пустым.
 */
void OrderBookModel::setDepth(int rows)
{
    m_depth = std::max(1, rows);
}
/**
 * @brief Вычисляет максимальное произведение цены и объёма (price * qty) среди всех уровней.
 *
 * Этот максимум нужен для масштабирования горизонтальных полосок объёма в стакане:
 * - Уровень с максимальным значением total получает полоску на всю ширину ячейки.
 * - Остальные уровни рисуются пропорционально: (levelTotal / maxTotal) * ширина ячейки.
 *
 * Пример:
 *   levels = { {price=100, qty=2}, {price=50, qty=10}, {price=200, qty=1} }
 *   total = 200, 500, 200 → максимум = 500.
 *
 * @param levels Вектор уровней (бидов или асков).
 * @return Максимальное значение price * qty. Если levels пуст, возвращает 0.0.
 */
double OrderBookModel::calcMaxTotal(const std::vector<OrderBookLevel>& levels)
{
    double maxTotal = 0.0;
    for (const OrderBookLevel& lv : levels)
    {
        double levelTotalPrice = lv.qty * lv.price;
        maxTotal = std::max(maxTotal, levelTotalPrice);
    }
    return maxTotal;
}
/**
 * @brief Находит максимальный объём (qty) среди всех уровней стакана.
 *
 * Используется для нормализации кружков объёма (VolumeCircle).
 * Максимальный объём определяет размер самого большого круга,
 * остальные круги масштабируются пропорционально.
 *
 * @param levels Вектор уровней (бидов или асков).
 * @return Максимальное значение qty, или 0.0 если вектор пуст.
 */
double OrderBookModel::calcMaxQty(const std::vector<OrderBookLevel>& levels)
{
    double maxTotalQty = 0.0;
    for (const OrderBookLevel& lv : levels)
        maxTotalQty = std::max(lv.qty, maxTotalQty);
    return maxTotalQty;
}//
