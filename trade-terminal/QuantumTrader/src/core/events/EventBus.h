#pragma once
#include<QObject>
#include<QString>
#include<vector>
#include"../models/Candle.h"
#include<utility>

#include "core/network/common/NetworkTypes.h"
#include "ui/charts/orderbook/OrderBookDataTypes.h"
#include "ui/charts/timeandsales/TimeAndSalesDataTypes.h"

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
	void deepHistoryReady(int chartId, const QString& exchange, const QString& symbol, const std::vector<Candle>& candles);	
	void liveCandleReceived(const QString& exchange, const QString& symbol,const ChartInterval& interval, const  Candle& candle);
	void symbolChanged(const QString& exchange, const QString& symbol, int linkGroupId = 0);
	void availableSymbolsLoaded(const QString& exchange, const QList<std::pair<QString, QString>>& symbols);
	void orderBookReceived(const QString& exchange, const QString& symbol, const OrderBookSnapshot& snapshot, bool isDelta);
	void tradeReceived(const QString& exchange, const QString& symbol, const TradeTick& tick);
private:
	explicit EventBus(QObject* parent = nullptr);
	~EventBus() override = default;
	
};
