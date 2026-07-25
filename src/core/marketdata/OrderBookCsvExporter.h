#pragma once

#include<QString>
#include<QVector>
#include"OrderBookHistoryManager.h"


class OrderBookCsvExporter
{
public:
	static bool exportToCsv(const QVector<OrderBookFeatureRow>& rows, const QString& filePath, qint64 horizonMs);
};
