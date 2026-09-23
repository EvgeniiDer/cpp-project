#include"MarketEventLog.h"
#include<Qfile>
#include<QTextstream>
#include<QStringList>
#include<QDebug>


MarketEventLog::MarketEventLog(const QString& logFilePath, qint64 retentionMs)
	:m_logFilePath(logFilePath), m_retentionMs(retentionMs)
{
	loadFromDisk();
}

QString MarketEventLog::typeToString(const MarketEventType& event)
{
	switch (event)
	{
	case MarketEventType::Sweep:
		return QStringLiteral("sweep");
	case MarketEventType::SpoofBurst:
		return QStringLiteral("spoof");
	}
	return QStringLiteral("unknown");
}
MarketEventType MarketEventLog::typeFromString(const QString& text)
{
	if (text == QStringLiteral("sweep"))
	{
		return MarketEventType::Sweep;
	}
	if (text == QStringLiteral("spoof"))
	{
		return MarketEventType::SpoofBurst;
	}
	// Неизвестный тип в файле (старый формат, ручная правка и т.п.) -
    // не валим загрузку всей истории из-за одной строки, просто
    // предупреждаем и берём дефолт.
	qWarning() << "[MarketEventLog] Unknown event type in log file: " << text;
	return MarketEventType::Sweep;
}

void MarketEventLog::loadFromDisk()
{
	QFile file(m_logFilePath);
	if (!file.exists())
	{
		//file not exists
		return;
	}
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "[MarketEventLog] Failed to open for reading: " << m_logFilePath;
		return;
	}

	QTextStream in(&file);
	while (!in.atEnd())
	{
		const QString line = in.readLine().trimmed();
		if (line.isEmpty())
		{
			continue;
		}

		const QStringList parts = line.split(',');
		if (parts.size() != kLogLineFieldCount)
		{
			qWarning() << "[MarketEventLog] Skipping malformed line: " << line;
			continue;
		}

		MarketEvent event;
		bool timestampOk = false;
		bool priceOk = false;
		bool magnitudeOk = false;

		event.timestamp = parts[0].toLongLong(&timestampOk);
		event.side = parts[1];
		event.type = typeFromString(parts[2]);
		event.price = parts[3].toDouble(&priceOk);
		event.magnitude = parts[4].toFloat(&magnitudeOk);

		if (!timestampOk || !priceOk || !magnitudeOk)
		{
			qWarning() << "[MarketEventLog] Skipping line with bad numbers:" << line;
			continue;
		}
		m_events.push_back(event);
	}
	file.close();


	// Файл мог копиться неделями - сразу отрезаем всё, что старше
	// retentionMs относительно момента загрузки (а не относительно
	// timestamp последней записи - именно "сейчас" нам и важно).
	if (!m_events.empty())
	{
		evictStale(m_events.back().timestamp);
	}

}

void MarketEventLog::appendToDisk(const MarketEvent& event) const
{
	QFile file(m_logFilePath);
	if (!file.open(QIODevice::Append | QIODevice::Text))
	{
		qWarning() << "[MarketEventLog] Failed to open for appending: " << m_logFilePath;
		return;
	}

	QTextStream out(&file);
	out << event.timestamp << ','
		<< event.side << ','
		<< typeToString(event.type) << ','
		<< event.price << ','
		<< event.magnitude << '\n';
	file.close();
}

void MarketEventLog::evictStale(qint64 currentTimestamp)
{
	const qint64 cutoff = currentTimestamp - m_retentionMs;
	while (!m_events.empty() && m_events.front().timestamp < cutoff)
	{
		m_events.pop_front();
	}
}

void MarketEventLog::record(const MarketEvent& event)
{
	m_events.push_back(event);
	evictStale(event.timestamp);
	appendToDisk(event);
}

int MarketEventLog::countNear(MarketEventType type, const QString& side, double price, double priceTolerance) const
{
	int count = 0;
	for (const MarketEvent& event : m_events)
	{
		if (event.type != type || event.side != side)
		{
			continue;
		}
		if (std::fabs(event.price - price) <= priceTolerance)
		{
			++count;
		}
	}
	return count;
}




