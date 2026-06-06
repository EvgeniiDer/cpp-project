#pragma once

#include<QObject>
#include<QString>
#include<QHash>
#include<functional>
#include<vector>

#include"../network/common/IExchangeConnector.h"
#include"../network/common/NetworkTypes.h"
#include"../models/Candle.h"

class CandleHistoryManager;
class MarketDataManager : public QObject
{
	Q_OBJECT
public:
	using ConnectorFactory = std::function <IExchangeConnector* (QObject* parent)>;
	explicit MarketDataManager(QObject* parent = nullptr);
	~MarketDataManager();

	void registerFactory(const QString& exchangeName, ConnectorFactory factory);
	void connectTo(const QString& exchangeName);

	void requestHistory(const MarketContext& ctx);
	void subscribeToStream(const MarketContext& ctx);
	void unsubscribeFromStream(const MarketContext& ctx);
	QList<std::pair<QString, QString>>getCachedSymbols(const QString& exchangeName)const;
signals:
	//TODO WHATCHLIST next!!!
	void tickerUpdated(const QString& exchangeName, const QString& symbol, double lastPrice, double volume);
private:
	QHash<QString, ConnectorFactory> m_factories;
	QHash<QString, IExchangeConnector*> m_activeConnectors;

	QHash<QString, CandleHistoryManager*>m_historyManagers;

	QHash<QString, QList<std::pair<QString,QString>>> m_cachedSymbols;

};
