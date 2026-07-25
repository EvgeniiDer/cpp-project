#include"OrderBookCsvExporter.h"
#include<QFile>
#include<QTextStream>
#include<QDateTime>


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

	QString rowToCsvLine(const OrderBookFeatureRow& row, int target)
	{
		QDateTime dt = QDateTime::fromMSecsSinceEpoch(row.timestamp);
		float hour = static_cast<float>(dt.time().hour());
		float dayOfWeek = static_cast<float>(dt.date().dayOfWeek());

		QStringList fields;
		fields << QString::number(row.bestBid, 'f', 4)
			<< QString::number(row.bestAsk, 'f', 4)
			<< QString::number(row.spread, 'f', 4)
			<< QString::number(row.imbalanceL1, 'f', 4)
			<< QString::number(row.imbalanceSma3, 'f', 4)
			<< QString::number(row.bidSpoofDrop, 'f', 4)
			<< QString::number(row.askSpoofDrop, 'f', 4)
			<< QString::number(row.bidFakeRatio, 'f', 4)
			<< QString::number(row.askFakeRatio, 'f', 4)
			<< QString::number(row.volumeDepthBids, 'f', 4)
			<< QString::number(row.volumeDepthAsks, 'f', 4)
			<< QString::number(hour, 'f', 0)
			<< QString::number(dayOfWeek, 'f', 0)
			<< QString::number(target);

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
	out << "best_bid,best_ask,spread,imbalance_l1,imbalance_sma3,"
		"bid_spoof_drop,ask_spoof_drop,bid_fake_ratio,ask_fake_ratio,"
		"vol_depth_bids,vol_depth_asks,hour,day_of_week,target\n";

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



