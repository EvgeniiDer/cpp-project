#include"BybitConnector.h"
#include<QNetworkAccessManager>
#include<QNetworkReply>
#include<QTimer>
#include<QWebSocket>
#include"ByBitParser.h"

#include<QUrlQuery>
#include<QPointer>
#include"../../events/EventBus.h"
BybitConnector::BybitConnector(QObject* parent) : IExchangeConnector(parent)
{
	m_manager = new QNetworkAccessManager(this);

	m_webSocket = new QWebSocket();
	//Пометкак QObject::connect вызываем через Родителя потому как у нас пееропределен метод connect!!!!!!! 
	QObject::connect(m_webSocket, &QWebSocket::connected, this, &BybitConnector::onWsConnected);
	QObject::connect(m_webSocket, &QWebSocket::disconnected, this, &BybitConnector::onWsDisconected);
	QObject::connect(m_webSocket, &QWebSocket::textMessageReceived, this, &BybitConnector::onWsTextMessageReceived);

	m_pingTimer = new QTimer(this);
	QObject::connect(m_pingTimer, &QTimer::timeout, this, &BybitConnector::sendWsPing);
}
/**
 * @brief Initiates a lightweight connection check to Bybit REST API V5.
 *
 * This method performs an asynchronous "ping" request to the public Bybit
 * endpoint `/v5/market/time` to validate network reachability and API
 * responsiveness before treating the connector as connected.
 *
 * Behavior:
 *  - Emits `stateChanged(ConnectionState::Connecting)` immediately to notify
 *    listeners that a connection attempt has started.
 *  - Issues an HTTP GET to `https://api.bybit.com/v5/market/time` using
 *    `m_manager` (`QNetworkAccessManager`).
 *  - Connects the returned `QNetworkReply`'s `finished` signal to a lambda
 *    which forwards the reply to `onPingFinished(reply)` for response parsing
 *    and final state updates.
 *
 * Important implementation details:
 *  - The request is asynchronous; `connect()` returns immediately.
 *  - `reply` is managed by Qt; `onPingFinished(...)` calls `reply->deleteLater()`
 *    after processing to release resources.
 *  - This routine only performs a REST "ping" / handshake. Opening/configuring
 *    the WebSocket (and subscribing to topics) is performed by other methods
 *    such as `subscribeQuotes()` and `onWsConnected()`.
 *
 * Error handling:
 *  - Network and API/JSON parsing failures are handled inside `onPingFinished`
 *    which will emit `stateChanged(ConnectionState::Error)` when appropriate.
 *
 * Threading:
 *  - Must be invoked from the thread that owns `m_manager` (normally the main
 *    / GUI thread). Qt network objects are not thread-safe across threads.
 *
 * Suggestions / TODOs:
 *  - Add explicit request timeout handling and optional retry/backoff logic.
 *  - Allow injecting a mockable `QNetworkAccessManager` for unit testing.
 */
void BybitConnector::connect()
{
	qDebug() << "[BybitConnector] Initiating a connection to Bybit Api V5... ";
	//EventBus 
	emit EventBus::instance().networkStatusChanged("Bybit", "Connecting...");

	QNetworkRequest request(QUrl("https://api.bybit.com/v5/market/time"));
	QNetworkReply* reply = m_manager->get(request);// обьект будет наполняться по мере их поступления
	
	QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]()
		{
			onPingFinished(reply);
		});
}
void BybitConnector::onPingFinished(QNetworkReply* reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{
		QString errorMsg;
		int retCode = ByBitParser::parserPingResponse(reply->readAll(), errorMsg);
		if (retCode == 0)
		{
			qDebug() << "[ByBitConnector] SUCCESS! Connection with Bybit established.";
			//EventBus
			emit EventBus::instance().networkStatusChanged("Bybit", "Connected");
		} else
		{
			qDebug() << "[ByBitConnector] API REJECTED! Reason:" << errorMsg;
			//EventBus
			emit EventBus::instance().networkStatusChanged("Bybit", "Error: " + errorMsg);
		}
	} else
	{
		qDebug() << "[ByBitConnector] NETWORK ERROR:" << reply->errorString();
		//EventBus
		emit EventBus::instance().networkStatusChanged("Bybit", "Network Error");
	}
	reply->deleteLater();
}
void BybitConnector::disconnect()
{
	qDebug() << "[BybitConnector] Disconnecting.....";
	m_pingTimer->stop();
	m_webSocket->close();

	//EventBus
	emit EventBus::instance().networkStatusChanged("Bybit", "Disconnected");
}
QString BybitConnector::intervalToByBitString(const ChartInterval& interval)const
{
	switch (interval.unit)
	{
		case ChartInterval::Unit::Minute:
			return QString::number(interval.count);
		case ChartInterval::Unit::Hour:
			return QString::number(interval.count * 60);
		case ChartInterval::Unit::Day:
			return "D";
		case ChartInterval::Unit::Week:
			return "W";
		case ChartInterval::Unit::Month:
			return "M";
		default: 
			return "1";
	}
}
void BybitConnector::fetchHistory(const QString& symbol, ChartInterval interval, int limit, qint64 endTime)
{
	QUrlQuery query;
	query.addQueryItem("category", "linear");
	query.addQueryItem("symbol", symbol);
	query.addQueryItem("interval", intervalToByBitString(interval));
	query.addQueryItem("limit", QString::number(limit));

	if (endTime > 0)
	{
		query.addQueryItem("end", QString::number(endTime));
	}
	QUrl url("https://api.bybit.com/v5/market/kline");
	url.setQuery(query);

	QNetworkRequest request(url);
	request.setTransferTimeout(10000);

	QNetworkReply* reply = m_manager->get(request);
	QPointer<BybitConnector> self(this);

	QObject::connect(reply, &QNetworkReply::finished, this, [self, reply, symbol]()
		{
			QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyGuard(reply);

			if (!self || reply->error() != QNetworkReply::NoError) return;
			std::vector<Candle>chunk = ByBitParser::parseHistory(reply->readAll());
			emit self->historyChunkLoaded(symbol, chunk);
		});
	
}
/**
 * @brief Subscribe to real-time market quotes for a given symbol using Bybit WebSocket v5.
 *
 * This routine:
 * - Sets `m_webSocket` properties `currentSymbol` and `currentInterval` which are used
 *   by WebSocket handlers to know what to subscribe to once the socket is open/connected.
 * - Currently uses a hard-coded test interval of 1 minute; `intervalToByBitString` converts
 *   the `ChartInterval` to Bybit's interval string format.
 * - Opens the public linear stream endpoint at `wss://stream.bybit.com/v5/public/linear`.
 *
 * Important side effects:
 * - Calls `m_webSocket->open(...)`, so ensure WebSocket signal handlers (connected/error/textMessageReceived, etc.)
 *   are connected prior to calling this function to handle subscription messages and errors.
 * - Does not send the actual Bybit subscription JSON payload after opening the socket;
 *   that should be implemented in the connection/connected handler (e.g. `onWsConnected`).
 *
 * @param symbol Trading pair symbol, e.g. "BTCUSDT".
 *
 * TODO:
 * - Replace the hard-coded `testInterval` with a caller-provided interval or connector configuration.
 * - Implement and send the subscription message (JSON) after the socket is connected.
 * - Add error handling and reconnection/backoff logic for production robustness.
 */
void BybitConnector::subscribeQuotes(const QString& symbol)
{
	ChartInterval testInterval(ChartInterval::Unit::Minute, 1);

	m_webSocket->setProperty("currentSymbol", symbol);
	m_webSocket->setProperty("currentInterval", intervalToByBitString(testInterval));

	QUrl wsUrl("wss://stream.bybit.com/v5/public/linear");
	qDebug() << "[Bybit WS] connection to" << wsUrl.toString();
	m_webSocket->open(wsUrl);
}
void BybitConnector::onWsConnected()
{
	qDebug() << "[Bybit WS] Connected! Launching pinger and subscribing to quotes...";
	m_pingTimer->start(20000);

	QString symbol = m_webSocket->property("currentSymbol").toString();
	QString interval = m_webSocket->property("currentInterval").toString();

	QString message = ByBitParser::buildSubscriptionRequest(symbol, interval);
	m_webSocket->sendTextMessage(message);
}
void BybitConnector::onWsTextMessageReceived(const QString& message)
{
	QString symbol;
	std::optional<Candle> liveCandle = ByBitParser::parseLiveCandle(message, symbol);
	if (liveCandle.has_value())
	{
		std::vector<Candle> liveUpdate = { liveCandle.value() };
		//EventBus
		emit EventBus::instance().liveCandleReceived("Bybit", symbol, liveCandle.value());
		qDebug() << "[LIVE]" << symbol << "Price:" << liveCandle->close;
	}
}
void BybitConnector::sendWsPing()
{
	QString message = ByBitParser::buildPinqRequest();
	m_webSocket->sendTextMessage(message);
}
void BybitConnector::onWsDisconected()
{
	qDebug() << "[Bybit WS] Disconnected. Error: " << m_webSocket->errorString();
	m_pingTimer->stop();
	//EventBus
	emit EventBus::instance().networkStatusChanged("Bybit", "Disconnected");
}
