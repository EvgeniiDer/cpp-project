#include"BybitConnector.h"
#include<QNetworkAccessManager>
#include<QNetworkReply>
#include<QTimer>
#include<QWebSocket>
#include"ByBitParser.h"
#include<utility>
#include<QUrlQuery>
#include<QPointer>
#include"../../events/EventBus.h"
#include<algorithm>
BybitConnector::BybitConnector(QObject* parent) : IExchangeConnector(parent)
{
	m_manager = new QNetworkAccessManager(this);

	m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
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

QString BybitConnector::getRestCategory(const QString& uiMarketType) const
{
	static const QHash<QString, QString> map = {
		{"SPOT",     "spot"},
		{"INV_PERP", "inverse"},
		{"OPTION",   "option"}
	};
	return map.value(uiMarketType, "linear");
}

QString BybitConnector::getWsEndpoint(const QString& uiMarketType) const
{
	static const QHash<QString, QString> map =
	{
		{"SPOT",     "wss://stream.bybit.com/v5/public/spot"},
		{"INV_PERP", "wss://stream.bybit.com/v5/public/inverse"},
		{"OPTION",   "wss://stream.bybit.com/v5/public/option"}
	};
	return map.value(uiMarketType, "wss://stream.bybit.com/v5/public/linear");
}

QString BybitConnector::getUiMarketType(const QString& category) const
{
	static const QHash<QString, QString> map =
	{
		{"spot",    "SPOT"},
		{"inverse", "INV_PERP"},
		{"option",  "OPTION"}
	};
	return map.value(category, "PERP");
}

void BybitConnector::fetchHistory(const MarketContext& ctx)
{
	QUrlQuery query;
	query.addQueryItem("category", getRestCategory(ctx.marketType));
	query.addQueryItem("symbol", ctx.symbol);
	query.addQueryItem("interval", intervalToByBitString(ctx.interval));
	query.addQueryItem("limit", QString::number(ctx.limit));

	if (ctx.endTime > 0)
	{
		query.addQueryItem("end", QString::number(ctx.endTime));
	}
	QUrl url("https://api.bybit.com/v5/market/kline");
	url.setQuery(query);

	QNetworkRequest request(url);
	request.setTransferTimeout(10000);

	QNetworkReply* reply = m_manager->get(request);
	QPointer<BybitConnector> self(this);
	
	QString targetSymbol = ctx.symbol;

	QObject::connect(reply, &QNetworkReply::finished, this, [self, reply, targetSymbol]()
		{
			QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyGuard(reply);

			if (!self) return;
			if (reply->error() != QNetworkReply::NoError)
			{
				QString errStr = reply->errorString();
				qWarning() << "[BybitConnector] fetchHistory error for: " << targetSymbol << "	Error: " << errStr;
				emit self->historyLoadFailed(targetSymbol, errStr);
				return;
			}
			std::vector<Candle>chunk = ByBitParser::parseHistory(reply->readAll());
			emit self->historyChunkLoaded(targetSymbol, chunk);
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
void BybitConnector::subscribeQuotes(const QString& symbol, const QString& marketType)
{
	ChartInterval testInterval(ChartInterval::Unit::Minute, 1);

	m_currentSymbol = symbol;
	m_currentInterval = intervalToByBitString(testInterval);
	m_currentMarketType = marketType;

	m_webSocket->setProperty("currentSymbol", m_currentSymbol);
	m_webSocket->setProperty("currentInterval", m_currentInterval);
	if (m_webSocket->state() != QAbstractSocket::UnconnectedState)
	{
		qDebug() << "[Bybit WS] Closing existing connection before switching...";
		m_webSocket->close();
	}
	QString wsUrl = getWsEndpoint(marketType);
	qDebug() << "[Bybit WS] Connecting to target market stream: " << wsUrl;
	m_webSocket->open(QUrl(wsUrl));
}
void BybitConnector::fetchAvailableSymbols()
{
	qDebug() << "[BybitConnector] Initiating parallel fetch for ALL 4 market categories...";
	
	struct SyncContext
	{
		QList <std::pair<QString, QString>> accumulateSymbols;
		int pendingRequests = 4;

	};
	
	std::shared_ptr<SyncContext> ctx = std::make_shared<SyncContext>();

	auto launchCategoryRequest = [this, ctx](const QString& category)
		{
			QUrl url("https://api.bybit.com/v5/market/instruments-info");
			QUrlQuery query;
			query.addQueryItem("category", category);
			url.setQuery(query);//example https://api.bybit.com/v5/market/instruments-info?category=spot

			QNetworkRequest request(url);
			request.setTransferTimeout(12000);

			QNetworkReply* reply = m_manager->get(request);
			/*
			* QPointer....
			  Это слабая ссылка на объект, производный от QObject.
		      Если объект будет удалён, self автоматически станет nullptr.
			  Позволяет безопасно проверить if (!self) return; – вдруг объект, который инициировал запрос, уже уничтожен.
			*/
			QPointer<BybitConnector> self(this);
			QObject::connect(reply, &QNetworkReply::finished, this, [self, reply, ctx, category]()
				{
				 /* QScopePointer<..>
				    Это аналог std::unique_ptr, но с дополнительными политиками удаления.
					При выходе replyGuard из области видимости (конец лямбды) он автоматически удаляет хранимый объект.
					Второй шаблонный аргумент QScopedPointerDeleteLater говорит: вместо delete вызвать deleteLater() (нужно для QObject).
				 */
					QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyGuard(reply);
					if (!self) return;
					if (reply->error() == QNetworkReply::NoError)
					{
						QStringList parsedSymbols = ByBitParser::parseAvailableSymbols(reply->readAll());
						QString marketType = self->getUiMarketType(category);
						for (const QString& symbol : parsedSymbols)
						{
							ctx->accumulateSymbols.append({symbol, marketType});
						}
					}
					ctx->pendingRequests--;
					if (ctx->pendingRequests == 0)
					{
						std::sort(ctx->accumulateSymbols.begin(), ctx->accumulateSymbols.end());
						emit EventBus::instance().availableSymbolsLoaded(ctx->accumulateSymbols);
					}
				});
		};
	launchCategoryRequest("spot");
	launchCategoryRequest("linear");
	launchCategoryRequest("inverse");
	launchCategoryRequest("option");
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
	std::optional<Candle> liveCandle = ByBitParser::parseLiveCandle(message.toUtf8(), symbol);
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
	QString message = ByBitParser::buildPingRequest();
	m_webSocket->sendTextMessage(message);
}
void BybitConnector::onWsDisconected()
{
	qDebug() << "[Bybit WS] Disconnected. Error: " << m_webSocket->errorString();
	m_pingTimer->stop();
	//EventBus
	emit EventBus::instance().networkStatusChanged("Bybit", "Disconnected");
}
