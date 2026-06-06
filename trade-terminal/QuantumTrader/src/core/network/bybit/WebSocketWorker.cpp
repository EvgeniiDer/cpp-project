#include"WebSocketWorker.h"
#include<QWebSocket>
#include<QTimer>
#include<QDebug>

WebSocketWorker::WebSocketWorker(const QString& workId, const QString& wsUrl, QObject* parent)
	:QObject(parent)
	,m_workerId(workId)
	,m_wsUrl(wsUrl)
{
	m_socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

	m_pingTimer = new QTimer(this);
	m_pingTimer->setInterval(Bybit::PING_INTERVAL_MS);

	m_pongWatchdog = new QTimer(this);
	m_pongWatchdog->setInterval(Bybit::PONG_TIMEOUT_MS);
	m_pongWatchdog->setSingleShot(true);

	QObject::connect(m_socket, &QWebSocket::connected, this, &WebSocketWorker::onConnected);
	QObject::connect(m_socket, &QWebSocket::disconnected, this, &WebSocketWorker::onDisconnected);
	QObject::connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocketWorker::onTextMessageReceived);
	QObject::connect(m_pingTimer, &QTimer::timeout, this, &WebSocketWorker::sendPing);
	QObject::connect(m_pongWatchdog, &QTimer::timeout, this, &WebSocketWorker::onPongTimeout);


}
WebSocketWorker::~WebSocketWorker()
{
	m_pingTimer->stop();
	m_pongWatchdog->stop();
	m_socket->close();
	qDebug() << "[Worker]" << m_workerId << "Destroyed.";
}

void WebSocketWorker::subscribe(const QString& topic, const QString& jsonPayload)
{
	if (m_subscriptions.contains(topic))
	{
		qDebug() << "[Worker]" << m_workerId << "Already subscribed to:" << topic;
		return;
	}
	m_subscriptions.insert(topic, jsonPayload);
	if (m_isConnected)
	{
		m_socket->sendTextMessage(jsonPayload);
		qDebug() << "[Worker]" << m_workerId << "Subscribed to topic:" << topic;
	}
	else
	{
		qDebug() << "[Worker]" << m_workerId << "Not connected yet. Topic queued:" << topic;
		if (m_socket->state() == QAbstractSocket::UnconnectedState)
		{
			qDebug() << "[Worker]" << m_workerId << "Opening socket...";
			m_socket->open(QUrl(m_wsUrl));
		}
	}
}

void WebSocketWorker::unsubscribe(const QString& topic, const QString& jsonPayload)
{
	if (!m_subscriptions.contains(topic))
	{
		qDebug() << "[Worker]" << m_workerId << "Not subscribed to:" << topic << "— skip.";
		return;
	}
	m_subscriptions.remove(topic);
	if (m_isConnected)
	{
		m_socket->sendTextMessage(jsonPayload);
		qDebug() << "[Worker]" << m_workerId << "Unsubscribed from topic:" << topic;
	}

}

//SLOTS
void WebSocketWorker::onConnected()
{
	m_isConnected = true;
	m_waitingForPong = false;

	m_pongWatchdog->stop();
	m_pingTimer->start();

	qDebug() << "[Worker]" << m_workerId << "Connected!"
		<< "Restoring" << m_subscriptions.size() << "subscription(s)...";

	emit statusChanged(m_workerId, "Connected");
	// Работает в том числе и при первом коннекте при рекконекте
	resubscribeAll();
}

void WebSocketWorker::onDisconnected()
{
	m_isConnected = false;
	m_waitingForPong = false;
	
	m_pingTimer->stop();
	m_pongWatchdog->stop();

	qWarning() << "[Worker]" << m_workerId
		<< "Disconnected. Error:" << m_socket->errorString();
	
	emit statusChanged(m_workerId, "Disconnected");
	scheduleReconnect();
}

void WebSocketWorker::onTextMessageReceived(const QString& message)
{
	//Pong
	if (message.contains(R"("op":"pong")") || message.contains(R"("ret_msg":"pong")"))
	{
		m_waitingForPong = false;
		m_pongWatchdog->stop();
		qDebug() << "[Worker]" << m_workerId << "Pong received. Connection alive.";
		return;
	}
	
	//system confirmations
	if(message.contains(R"("op":"subscribe")") || message.contains(R"("op":"unsubscribe")"))
	{
		qDebug() << "[Worker]" << m_workerId << "System message:" << message;
		return;
	}
	emit messageReceived(m_workerId, message);
}

void WebSocketWorker::sendPing()
{
	if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState)
	{
		qWarning() << "[Worker]" << m_workerId << "sendPing: socket not connected, skip.";
		return;
	}
	if (m_waitingForPong)
	{
		qWarning() << "[Worker]" << m_workerId
			<< "Zombie detected! No pong since last ping. Reconnecting...";
		forceReconnect();
		return;
	}
	m_socket->sendTextMessage(R"({"op":"ping"})");
	m_waitingForPong = true;
	m_pongWatchdog->start();

	qDebug() << "[Worker]" << m_workerId << "Ping sent. Waiting for pong...";
}

void WebSocketWorker::onPongTimeout()
{
	if (m_waitingForPong)
	{
		qWarning() << "[Worker]" << m_workerId
			<< "PONG TIMEOUT! Server silent for"
			<< Bybit::PONG_TIMEOUT_MS << "ms. Forcing reconnect...";
		forceReconnect();
	}
}

//Private methods
void WebSocketWorker::scheduleReconnect()
{
	if (m_subscriptions.isEmpty())return;
	qDebug() << "[Worker]" << m_workerId << "Has" << m_subscriptions.size() << "active sub(s). Reconnecting in" << Bybit::RECONNECT_DELAY_MS << "ms...";

		QTimer::singleShot(Bybit::RECONNECT_DELAY_MS, this, [this]()
			{
				qDebug() << "[Worker]" << m_workerId << "Attempting reconnect...";
				m_socket->open(QUrl(m_wsUrl));
			});
}

void WebSocketWorker::resubscribeAll()
{
	if (m_subscriptions.isEmpty())return;
	for (QHash<QString, QString>::const_iterator it = m_subscriptions.begin(); it != m_subscriptions.end(); ++it)
	{
		m_socket->sendTextMessage(it.value());
		qDebug() << "[Worker]" << m_workerId << "Restored subscription:" << it.key();
	}
}

void WebSocketWorker::forceReconnect()
{
	m_pingTimer->stop();
	m_pongWatchdog->stop();
	m_waitingForPong = false;
	m_isConnected = false;
	qDebug() << "[Worker]" << m_workerId
		<< "Force reconnect. Preserving" << m_subscriptions.size() << "sub(s).";

	m_socket->close();
	m_socket->open(QUrl(m_wsUrl));

}
