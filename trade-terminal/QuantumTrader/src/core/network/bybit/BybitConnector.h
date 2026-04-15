#pragma once

#include"../common/IExchangeConnector.h"

class QNetworkAccessManager;
class QNetworkReply;
class QWebSocket;
class QTimer;


class ByBitConnector : public IExchangeConnector
{
	Q_OBJECT
public:
	explicit ByBitConnector(QObject* parent = nullptr);
private slots:
	//void onPingFinished(QNetworkReply* reply);
	void onWsConnected();
	//void onWsDisconected();
	//void onWsTextMessageReceived(const QString& message);
	//void sendWsPing();
private:
	QWebSocket* m_webSocket;
	QNetworkAccessManager* m_manager;
	QTimer* m_pingTimer;
};