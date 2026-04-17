#include"BybitConnector.h"
#include<QNetworkAccessManager>
#include<QNetworkReply>
#include<QTimer>
#include<QWebSocket>

#include<QJsonDocument>
#include<QJsonValue>
#include<QJsonObject>
#include<QUrlQuery>
#include<QJsonArray>
BybitConnector::BybitConnector(QObject* parent) : IExchangeConnector(parent)
{
	m_manager = new QNetworkAccessManager(this);

	m_webSocket = new QWebSocket();
	//connect(m_webSocket, &QWebSocket::connected, this, &ByBitConnector::onWsConnected());
}

void BybitConnector::connect()
{
	qDebug() << "[BybitConnector] Initiating a connection to Bybit Api V5... ";
	emit stateChanged(ConnectionState::Connecting);


	QNetworkRequest request(QUrl("https://api.bybit.com/v5/market/time"));
	QNetworkReply* reply = m_manager->get(request);
	
	QObject::connect(reply, QNetworkReply::finished, this, [this, reply]()
		{
			onPingFinished(reply);
		});
}
void BybitConnector::onPingFinished(QNetworkReply* reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{
		QByteArray responseData = reply->readAll();
		QJsonParseError parseError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);
		
		if (jsonDoc.isNull() || parseError.error != QJsonParseError::NoError)
		{
			qDebug() << "[ByBitConnector] CRITICAL ERROR parsing JSON: " << parseError.errorString();
			qDebug() << "[BybitConnector] Raw response : " << responseData;
			emit stateChanged(ConnectionState::Error);
			reply->deleteLater();
			return;
		}

		if (jsonDoc.isObject())
		{
			QJsonObject root = jsonDoc.object();
			int retCode = root.value("retCode").toInt();

			if (retCode == 0)
			{
				qDebug() << "[ByBitConnector] SUCCESS! Connection with Bybit established.";
				emit stateChanged(ConnectionState::Connected);
			} else
			{
				QString errorMsg = root.value("retMsg").toString();
				qDebug() << "[ByBitConnector] API REJECTED! Code:" << retCode << "Reason:" << errorMsg;
				emit stateChanged(ConnectionState::Error);
			}
		}

	} else
	{
		qDebug() << "[ByBitConnector] NETWORK ERROR:" << reply->errorString();
		emit stateChanged(ConnectionState::Error);
	}
	reply->deleteLater();
}
void BybitConnector::disconnect()
{
	qDebug() << "[BybitConnector] Disconnecting.....";
	m_pingTimer->stop();
	m_webSocket->close();

	emit stateChanged(ConnectionState::Disconnected);
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
/**
 * @brief Fetch historical K-line (candlestick) data from Bybit REST API v5.
 *
 * Notes:
 * - The Bybit V5 K-line endpoint does not support Tick or Range intervals.
 *   These interval types must be produced by aggregating public trades (trade stream).
 * - Bybit V5 limits the number of candles per request (typically max 1000).
 *
 * @param symbol   Trading pair symbol, e.g. "BTCUSDT".
 * @param interval Timeframe to request. Uses `ChartInterval` (Minute/Hour/Day/Week/Month).
 *                 For Hour and Minute the function converts to Bybit's minute-based interval string.
 *                 Tick and Range units are not supported and will be rejected by this function.
 * @param limit    Number of candles to retrieve (server-enforced maximum; see Bybit docs).
 *
 * Behavior:
 * - Builds a GET request to `https://api.bybit.com/v5/market/kline` with query parameters:
 *   `category=linear`, `symbol`, `interval` and `limit`.
 * - On successful HTTP + API response (`retCode == 0`) the function extracts `result.list`
 *   (an array of candles) and logs the most recent closed candle price.
 * - On failure it logs the relevant API or network error.
 *
 * TODO:
 * - Parse `result.list` into the project's internal candle structure, pass it to `BybitParser`
 *   and emit `candlesReceived(...)`.
 */
void BybitConnector::fetchHistory(const QString& symbol, ChartInterval interval, int limit)
{
	qDebug() << "[BybitConnector] Request history (" << limit << "candles) for" << symbol;

	if(interval.unit == ChartInterval::Unit::Tick || interval.unit == ChartInterval::Unit::Range)
	{ 
		qDebug() << "[BybitConnector] ERROR: Ticks and Ranges are not yet supported in fetchHistory!";
		return;
	}
	QUrl url("https://api.bybit.com/v5/market/kline");//эндпоинт(Endpoint)
	QUrlQuery query;
	query.addQueryItem("category", "linear");
	query.addQueryItem("symbol", symbol);
	query.addQueryItem("interval", intervalToByBitString(interval));
	query.addQueryItem("limit", QString::number(limit));
	url.setQuery(query);

	QNetworkRequest request(url);
	QNetworkReply* reply = m_manager->get(request);

	QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, symbol]()
		{
			if (reply->error() == QNetworkReply::NoError)
			{
				QByteArray data = reply->readAll();
				QJsonDocument doc = QJsonDocument::fromJson(data);
				QJsonObject root = doc.object();

				if (root.value("retCode").toInt() == 0)
				{
					QJsonArray list = root.value("result").toObject().value("list").toArray();
					qDebug() << "[BybitConnector] SUCCESS! Received historical candles:" << list.size() << "for" << symbol;

					if (!list.isEmpty())
					{
						QJsonArray latestCandle = list.first().toArray();
						qDebug() << "[HISTORY]" << symbol << "Last Closed Price:" << latestCandle[4].toString();
					}
					// TODO: Передать данные в BybitParser и вызвать emit candlesReceived(...)
				} else
				{
					qDebug() << "[BybitConnector] API Error:" << root.value("retMsg").toString();
				}
			} else
			{
				qDebug() << "[BybitConnector] Network error (REST):" << reply->errorString();
			}
			reply->deleteLater();
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
	qDebug() << "[Bybit WS] connectin to" << wsUrl.toString();
	m_webSocket->open(wsUrl);
}
void BybitConnector::onWsConnected()
{
	qDebug() << "[Bybit WS] Connected! Launching pinger and subscribing to quotes...";
	m_pingTimer->start(20000);

	QString symbol = m_webSocket->property("currentSymbol").toString();
	QString interval = m_webSocket->property("currentInterval").toString();

	QString topic = QString("kline.%1.%2").arg(interval).arg(symbol);

	QJsonObject req;
	req["req_id"] = "sub_" + symbol;
	req["op"] = "subscribe";
	req["args"] = QJsonArray{ topic };

	m_webSocket->sendTextMessage(QJsonDocument(req).toJson(QJsonDocument::Compact));
}
