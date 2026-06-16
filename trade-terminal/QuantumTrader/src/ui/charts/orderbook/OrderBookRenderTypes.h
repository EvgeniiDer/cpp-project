#pragma once
#include<QColor>
#include"OrderBookDataTypes.h"


struct OrderBookColors
{
    QColor bgColor     { 0x14, 0x16, 0x1a };
    QColor askColor    { 0xf6, 0x46, 0x5d };
    QColor bidColor    { 0x0e, 0xcb, 0x81 };
    QColor askBg       { 0xf6, 0x46, 0x5d, 30 };
    QColor bidBg       { 0x0e, 0xcb, 0x81, 30 };
    QColor textColor   { 0xd1, 0xd5, 0xdb };
    QColor headerColor { 0x6b, 0x72, 0x80 };
    QColor spreadBg    { 0x1f, 0x22, 0x28 };
};
/**
 * @brief Круг объёма для визуализации относительного размера заявки.
 *
 *  Проблема: если рисовать круги с радиусом, пропорциональным абсолютному объёму,
 *  то самый большой объём даст круг на всю клетку, а самый маленький будет размером с точку
 *  (или даже невидим). Нормализация по максимальному объёму фиксит это.
 *
 * @var qty      Текущий объём заявки на уровне (может быть изменён по приходу дельты).
 * @var maxQty   Максимальный объём среди всех уровней данной стороны (ask или bid).
 * @var isAsk    Флаг стороны: true = красный круг (аск, продажа), false = зелёный (бид, покупка).
 */
struct VolumeCircle// ебучие кружки обьема 
{
	double qty = 0.0; //Volume
	double maxQty = 0.0; 
	bool isAsk = false;
// Нормализация радиуса не было проблем с маленьки и большим радиуосом 
	float normalizedRadius() const
	{
		if (maxQty <= 0) return 0.0f;
		return static_cast<float>(std::min(1.0, qty / maxQty));
	}
};
/**
 * @brief Контекст для отрисовки одной строки (уровня) стакана.
 *
 *  Собирает в кучу все данные, которые нужны методу drawRow(), чтобы нарисовать одну строку:
 *  цену, объём, сумму, цветной бар объёма и кружок.
 *  Использование одной структуры вместо кучи параметров упрощает код и расширение.
 *
 * @var level     Ссылка на уровень стакана (цена + объём). Константная, чтобы не копировать.
 * @var maxTotal  Максимальная сумма (price * qty) среди всех уровней текущей стороны.
 *                Нужна для масштабирования горизонтальных баров объёма (отношение total / maxTotal).
 * @var isAsk     Флаг стороны: true = аск (продажа), false = бид (покупка).
 *                Определяет цвет текста, цвет бара объёма, цвет кружка (через VolumeCircle).
 * @var circle    Круг объёма (VolumeCircle) с уже посчитанными qty, maxQty и флагом isAsk.
 *                Его метод normalizedRadius() даст коэффициент от 0 до 1 для радиуса кружка.
 */
struct RowContext
{
	const OrderBookLevel& level;
	double maxTotal = 0.0;
	bool isAsk = false;

	VolumeCircle circle;
};

struct OrderBookRenderConfig
{
	float circleColW = 40.f;
	// Сюда в будущем:
	// bool  showCumulative = false;
	// bool  showDelta      = false;   // дельта bid/ask давление
	// int   heatmapLevels  = 0;       // тепловая карта объёмов
};