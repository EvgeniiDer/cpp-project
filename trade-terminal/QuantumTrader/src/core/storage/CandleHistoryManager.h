#pragma once

#include<QObject>
#include<QString>
#include<vector>
#include<memory>

#include "../models/Candle.h"
#include "../network/common/IExchangeConnector.h"
#include "../network/common/NetworkTypes.h"


class CandleHistoryManager : public QObject
{
	Q_OBJECT
public:
	explicit CandleHistoryManager(IExchangeConnector* connector, const QString& exchangeName, QObject* parent = nullptr);
	~CandleHistoryManager()override = default;
	void loadDeepHistory(const MarketContext& ctx);
	bool exportHistoryToCsv(const QString& filePath)const;
signals:
	void historyReady(const QString& symbol, const std::vector<Candle>& fullHistory);
	void historyChunkAppended(const QString& symbol, int currentSize, int targetSize);
private slots:
	void onChunkLoaded(int chartId, const QString& symbol, const std::vector<Candle>& chunk);
private:
	IExchangeConnector* m_connector;
	std::vector<Candle> m_accumulated;

	QString m_exchangeName;
	QString m_currentSymbol;
	QString m_currentMarketType;
	ChartInterval m_currentInterval;
	int m_targetLimit = 0;
};

