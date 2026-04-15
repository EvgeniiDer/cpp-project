#include<QNetworkAccessManager>
#include<QNetworkReply>
#include<QTimer>
#include<QWebSocket>

ByBitConnector::ByBitConnector(QObject* parent) : IExchangeConnector(parent)
{
	m_manager = new QNetworkAccessManager(this);

	m_webSocket = new QWebSocket();
	
	connect(m_webSocket, &QWebSocket::connected, this, &ByBitConnector::onWsConnected());
}

void ByBitConnector::onWsConnected()
{}
voi
