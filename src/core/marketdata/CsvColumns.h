#pragma once
#include <QString>
#include <QStringList>
#include <functional>
#include <type_traits>
#include <vector>
#include <QDateTime>
#include "types/OrderBookFeatureRow.h"


struct CsvColumn
{
	QString name;
	std::function<QString(const OrderBookFeatureRow&)> getter;
};


// Фабрика CSV-колонок для OrderBookFeatureRow.
//
// ЗАЧЕМ: чтобы не писать для каждого поля свою лямбду с форматированием
// (копипаста, легко ошибиться, трудно менять формат). Формат описан в одном месте,
// а добавление колонки это одна строка.
//
// ПРИМЕР
//   struct OrderBookFeatureRow {
//       qint64 timestamp;
//       double spread;
//       double imbalance;
//       int    bidLevels;
//   };
//
//   QVector<CsvColumn> columns = {
//       make("timestamp", &OrderBookFeatureRow::timestamp),  // T = qint64 -> целое
//       make("spread",    &OrderBookFeatureRow::spread),     // T = double -> 4 знака
//       make("imbalance", &OrderBookFeatureRow::imbalance),  // T = double -> 4 знака
//       make("bidLevels", &OrderBookFeatureRow::bidLevels),  // T = int    -> целое
//   };
//
//   // заголовок CSV: берём name
//   for (const auto& c : columns) header << c.name;
//
//   // строка данных: вызываем getter для конкретной строки
//   OrderBookFeatureRow row{1000, 0.0123, 0.55, 5};
//   for (const auto& c : columns) line << c.getter(row);
//   // результат: 1000;0.0123;0.5500;5
//
// КАК РАБОТАЕТ
//  - T выводится компилятором автоматически из типа поля (double, int, qint64...)
//  - member это указатель на поле структуры, а не на конкретный объект.
//    Это "ярлык": какое поле читать. Значение достаётся при обращении: row.*member
//    (один и тот же member работает с любой строкой: a.*member, b.*member)
//  - лямбда запоминает member в захвате [member] и вызывается для каждой строки
//  - if constexpr выбирает формат при компиляции:
//        floating point -> 4 знака после запятой
//        целые          -> как есть
//    Неподходящая ветка в итоговый код не попадает (с обычным if не скомпилировалось бы).
//  - тип поля фиксирован структурой, поэтому "подать int вместо double" в рантайме
//    невозможно. Меняешь тип поля в структуре и пересобираешь: формат подстроится сам.
//
// ВОЗВРАЩАЕТ: CsvColumn { name, getter }
//  - name   : заголовок колонки в CSV
//  - getter : std::function<QString(const OrderBookFeatureRow&)>, то есть сигнатура
//             "принимает строку данных, возвращает текст". std::function стирает тип
//             конкретной лямбды, поэтому колонки разных типов лежат в одном QVector.
//
// ЕСЛИ ПОНАДОБИТСЯ другой формат (например, 6 знаков): менять только 'f', 4 ниже.
namespace CsvColumns
{
	template<typename T>
	CsvColumn make(const QString& name, T OrderBookFeatureRow::* member)
	{
		return
		{
			name,
			[member](const OrderBookFeatureRow& row)
			{
				if constexpr (std::is_floating_point_v<T>)
				{
					return QString::number(row.*member, 'f', 4);
				}
				else
				{
					return QString::number(row.*member);
				}
			}

		};
	}
	inline QString header(const std::vector<CsvColumn>& columns)
	{
		QStringList names;
		for (const CsvColumn& c : columns)
		{
			names << c.name;
		}
		return names.join(",");
	}

	inline void appendValues(QStringList& fields, const std::vector<CsvColumn>& columns, const OrderBookFeatureRow& row)
	{
		for (const CsvColumn& c : columns)
		{
			fields << c.getter(row);
		}
	}
	template<typename Extract>
	CsvColumn timeColumn(const QString& name, Extract extract)
	{
		return
		{
			name,
			[extract](const OrderBookFeatureRow& row)
			{
				return QString::number(extract(QDateTime::fromMSecsSinceEpoch(row.timestamp)));
			}
		};
	}
	
}
