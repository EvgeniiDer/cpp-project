#pragma once

#include"../common/IExchangeConnector.h"

class QNetworkAccessManager;
class QNetworkReply;
class QWebSocket;
class QTimer;


class BybitConnector : public IExchangeConnector
{
	Q_OBJECT
public:
	explicit BybitConnector(QObject* parent = nullptr);
	void connect()override;
	void disconnect()override;
	void fetchHistory(const QString& symbol, ChartInterval interval, int limit, qint64 endTime = 0)override;
	void subscribeQuotes(const QString& symbol)override;
private slots:
	void onPingFinished(QNetworkReply* reply);
	void onWsConnected();
	void onWsDisconected();
	void onWsTextMessageReceived(const QString& message);
	void sendWsPing();
private:
	QWebSocket* m_webSocket;
	QNetworkAccessManager* m_manager;
	QTimer* m_pingTimer;
	QString intervalToByBitString(const ChartInterval& interval)const;
};