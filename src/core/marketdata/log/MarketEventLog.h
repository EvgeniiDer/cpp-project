#pragma once
#include<QString>
#include<deque>

enum class MarketEventType : std::uint8_t
{
	Sweep,
	SpoofBurst,
};

struct MarketEvent
{
	qint64 timestamp = 0; //(mc)
	QString side; //bid ask
	double price = 0.0; //price
	MarketEventType type = MarketEventType::Sweep;
	float magnitude = 0.0f; //насколько большое пока в разработке!!
};

class MarketEventLog
{
public:
	explicit MarketEventLog(const QString& logFilePath, qint64 retentionMs);
	void record(const MarketEvent& event);

	[[nodiscard]] int countNear(MarketEventType type, const QString& side, double price, double priceTolerance)const;
private:
	void loadFromDisk();
	void appendToDisk(const MarketEvent& event)const;
	void evictStale(qint64 currentTimestamp);

	static QString typeToString(const MarketEventType& event);
	static MarketEventType typeFromString(const QString& text);

	static constexpr int kLogLineFieldCount = 5;

	QString m_logFilePath;
	qint64 m_retentionMs; // СРОК ХРАНЕНИЯ НЕ УСТАНОВЛЕН УСТАНОВИТЬ!!!!!!
	std::deque<MarketEvent> m_events;

};