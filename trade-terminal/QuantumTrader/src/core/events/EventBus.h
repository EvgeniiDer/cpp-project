#pragma once
#include<QObject>
#include<QString>
#include<vector>
#include"../models/Candle.h"
class EventBus : public QObject
{
	Q_OBJECT
public:
	static EventBus& instance()
	{
		static EventBus bus;
		return bus;
	}

	EventBus(const EventBus&) = delete;
	EventBus& operator=(const EventBus&) = delete;
signals:
	void networkStatusChanged(const QString& exchange, const QString& status);
	void deepHistoryReady(const QString& exchange, const QString& symbol, const std::vector<Candle>& candles);	
	void liveCandleReceived(const QString& exchange, const QString& symbol, const  Candle& candle);
	void symbolChanged(const QString& exchange, const QString& symbol, int linkGroupId = 0);
private:
	explicit EventBus(QObject* parent = nullptr) : QObject(parent){}
	~EventBus() override = default;
	
};