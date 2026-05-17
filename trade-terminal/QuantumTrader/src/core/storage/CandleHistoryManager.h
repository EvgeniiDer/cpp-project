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
	explicit CandleHistoryManager(IExchangeConnector* connector, QObject* parent = nullptr);
	void loadDeepHistory(const QString& symbol, ChartInterval interval, int targetLimit);
signals:
	void historyReady(const QString& symbol, const std::vector<Candle>& fullHistory);
public slots:
	void onChunkLoaded(const QString& symbol, const std::vector<Candle>& chunk);
private:
	IExchangeConnector* m_connector;
	std::vector<Candle> m_accumulated;

	QString m_currentSymbol;
	ChartInterval m_currentInterval;
	int m_targetLimit = 0;
};

