#include"OrderBookCsvExporter.h"
#include<QFile>
#include<QTextStream>
#include<QDateTime>
#include"CsvColumns.h"

namespace
{
	int computeTarget(const QVector<OrderBookFeatureRow>& rows, int currentIndex, qint64 horizonMs)
	{
		const OrderBookFeatureRow& current = rows[currentIndex];
		const qint64 targetTs = current.timestamp + horizonMs;
		/*============================================================
		std::lower_bound с кастомным компаратором — краткая памятка
		============================================================

		1. Сигнатура:
		   auto it = std::lower_bound(first, last, value, comp);

		2. Что делает:
		   - Ищет первый элемент в диапазоне [first, last), который НЕ МЕНЬШЕ value.
		   - Диапазон должен быть ОТСОРТИРОВАН по правилу comp.

		3. Компаратор comp — это бинарный предикат (2 аргумента):
		   - Первый аргумент — элемент контейнера.
		   - Второй аргумент — значение value (искомое).
		   - Возвращает true, если элемент СТРОГО МЕНЬШЕ value.
		   - В нашем случае: comp(row, ts) = row.timestamp < ts.

		4. Как работает:
		   - Алгоритм выполняет бинарный поиск, используя comp for (int i = 0; i < rows.size() - 1; ++i)
	{
		int target = computeTarget(rows[i], rows[i + 1]);
		out << rowToCsvLine(rows[i], target);
	}
для сравнения.
		   - Если comp(mid, value) == true -> идём вправо.
		   - Если false -> идём влево (или нашли границу).

		5. Возвращаемое значение:
		   - Итератор на первый элемент, для которого comp(element, value) == false.
		   - Если все элементы < value, возвращается last.

		6. Пример из кода:
		   Ищем первый снимок, время которого >= targetTs.
		   Компаратор: row.timestamp < ts (где ts = targetTs).

		============================================================
		Запомнить:
		- ts внутри лямбды — это НЕ захваченная переменная, а параметр,
		  которому алгоритм подставляет value при каждом вызове.
		- Никаких трёх аргументов! Компаратор строго бинарный.
		============================================================*/
		QVector<OrderBookFeatureRow>::const_iterator it = std::lower_bound(
			rows.begin() + currentIndex + 1,
			rows.end(),
			targetTs,
			[](const OrderBookFeatureRow& row, qint64 ts)
			{
				return row.timestamp < ts;
			});
		if (it == rows.end())
		{
			return -1;
		}
		return(it->midPrice > current.midPrice) ? 1 : 0;
	}
	const std::vector<CsvColumn>& featureColumns()
	{
		static const std::vector<CsvColumn> columns = {
		 CsvColumns::make("best_bid",        &OrderBookFeatureRow::bestBid),
		CsvColumns::make("best_ask",        &OrderBookFeatureRow::bestAsk),
		CsvColumns::make("spread",          &OrderBookFeatureRow::spread),
		CsvColumns::make("imbalance_l1",    &OrderBookFeatureRow::imbalanceL1),
		CsvColumns::make("imbalance_sma3",  &OrderBookFeatureRow::imbalanceSma3),
		CsvColumns::make("bid_spoof_drop",  &OrderBookFeatureRow::bidSpoofDrop),
		CsvColumns::make("ask_spoof_drop",  &OrderBookFeatureRow::askSpoofDrop),
		CsvColumns::make("bid_fake_ratio",  &OrderBookFeatureRow::bidFakeRatio),
		CsvColumns::make("ask_fake_ratio",  &OrderBookFeatureRow::askFakeRatio),
		CsvColumns::make("vol_depth_bids",  &OrderBookFeatureRow::volumeDepthBids),
		CsvColumns::make("vol_depth_asks",  &OrderBookFeatureRow::volumeDepthAsks),

		CsvColumns::timeColumn("hour", [](const QDateTime& dt)
		{
			return dt.time().hour();
		}),
	    CsvColumns::timeColumn("day_of_week", [](const QDateTime& dt)
	    {
			return dt.date().dayOfWeek();
	    }),
		CsvColumns::timeColumn("minute",      [](const QDateTime& dt)
		{
			return dt.time().minute();
		}),


		CsvColumns::make("bid_sweep_levels",        &OrderBookFeatureRow::bidSweepLevels),
		CsvColumns::make("ask_sweep_levels",        &OrderBookFeatureRow::askSweepLevels),
		CsvColumns::make("bid_sweep_qty",           &OrderBookFeatureRow::bidSweepQty),
		CsvColumns::make("ask_sweep_qty",           &OrderBookFeatureRow::askSweepQty),
		CsvColumns::make("bid_sweep_repeat_wide",   &OrderBookFeatureRow::bidSweepRepeatCount),
		CsvColumns::make("bid_sweep_repeat_narrow", &OrderBookFeatureRow::bidSweepRepeatCountNarrow),
		CsvColumns::make("ask_sweep_repeat_wide",   &OrderBookFeatureRow::askSweepRepeatCount),
		CsvColumns::make("ask_sweep_repeat_narrow", &OrderBookFeatureRow::askSweepRepeatCountNarrow)
		};
		return columns;
	}
	QString rowToCsvLine(const OrderBookFeatureRow& row, int target)
	{
		QStringList fields;
		CsvColumns::appendValues(fields, featureColumns(), row);
		fields << QString::number(target);
		return fields.join(",") + "\n";
	}
}

bool OrderBookCsvExporter::exportToCsv(const QVector<OrderBookFeatureRow>& rows, const QString& filePath, qint64 horizonMs)
{
	if (rows.size() < 2) return false;
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		return false;
	}

	QTextStream out(&file);
    out << CsvColumns::header(featureColumns())
    << ",target\n";

	for (int i = 0; i < rows.size(); ++i)
	{
		int target = computeTarget(rows, i, horizonMs);
		if (target < 0)
		{
			continue; //данных нету
		}
		out << rowToCsvLine(rows[i], target);
	}

	if (out.status() != QTextStream::Ok)
	{
		return false;
	}
	file.close();
	return (file.error() == QFile::NoError);
}



