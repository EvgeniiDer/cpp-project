#pragma once
#include "OrderBookBase.h"

/**
 * @brief Классический вид стакана (Binance-стиль).
 *
 * Единый прокручиваемый список:
 *   [ Header                          ]
 *   [ Worst Ask (самая высокая цена)  ]  ↑ колёсиком вверх — видим глубже
 *   [ ...                             ]
 *   [ Best Ask                        ]
 *   [ Spread                          ]
 *   [ Best Bid                        ]
 *   [ ...                             ]
 *   [ Worst Bid (самая низкая цена)   ]  ↓ колёсиком вниз — видим глубже
 *
 * Скролл — колёсиком мыши (3 строки за тик, Shift = 1 строка).
 * Ресайз колонок — тяни границу между Price|Size или Size|Total.
 *
 * Отвечает ТОЛЬКО за компоновку. Данные, мерж, расчёты — в OrderBookModel/Base.
 */
class OrderBookClassic : public OrderBookBase
{
    Q_OBJECT
public:
    explicit OrderBookClassic(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event)  override;
    void resizeEvent(QResizeEvent* event) override;
};
