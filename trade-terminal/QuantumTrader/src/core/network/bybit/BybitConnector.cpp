#include"BybitConnector.h"
#include"ByBitParser.h"
#include"../../src/core/events/EventBus.h"
#include<QNetworkAccessManager>
#include<QNetworkReply>
#include<QUrlQuery>
#include<QPointer>
#include<QDebug>
#include<algorithm>

//Конфигурация Bybit — все URL, таймауты и таблицы типов рынка в одном месте

namespace BybitConfig
{
	struct MarketTypeInfo
	{
		const char* restCategory; //"spot" , "linear" and s.n.
		const char* wsEndpoint; // "wss://...
		const char* uiName; // "SPOT", "PERP"....
	};
	
	inline const QHash<QString, MarketTypeInfo> MARKET_TYPE_MAP = {
			{"SPOT",     {"spot",    "wss://stream.bybit.com/v5/public/spot",     "SPOT"}},
			{"INV_PERP", {"inverse", "wss://stream.bybit.com/v5/public/inverse",  "INV_PERP"}},
			{"OPTION",   {"option",  "wss://stream.bybit.com/v5/public/option",   "OPTION"}},
			{"PERP",     {"linear",  "wss://stream.bybit.com/v5/public/linear",   "PERP"}},
	};
	
	inline const MarketTypeInfo DEFAULT_MARKET = {
	   "linear",
	   "wss://stream.bybit.com/v5/public/linear",
	   "PERP"
	};
	//REST endpoints Почему char а не QString к примеру или std::string
	// проблема в том что constextp требует что бы значение было известно в процессе компиляции
	// QString выделяет память в кучи и это runtime операция 
	constexpr const char* REST_BASE_URL = "https://api.bybit.com";
	constexpr const char* REST_PING = "/v5/market/time";
	constexpr const char* REST_KLINE = "/v5/market/kline";
	constexpr const char* REST_INSTRUMENTS = "/v5/market/instruments-info";

	//WebSocket endpoints
	constexpr const char* WS_LINEAR = "wss://stream.bybit.com/v5/public/linear";
	constexpr const char* WS_SPOT = "wss://stream.bybit.com/v5/public/spot";
	constexpr const char* WS_INVERSE = "wss://stream.bybit.com/v5/public/inverse";
	constexpr const char* WS_OPTION = "wss://stream.bybit.com/v5/public/option";

	//Rest settings
	constexpr int REST_HISTORY_TIMEOUT_MS = 10000;
	constexpr int REST_SYMBOLS_TIMEOUT_MS = 12000;

	//OrderBook depth
	constexpr int ORDERBOOK_DEPTH = 50;

	//Категория для паралельной загрузки символвов
	inline const QStringList SYMBOL_CATEGORIES = { "spot", "linear", "inverse", "option" };

}
//Constructor
/**
 * @brief Создаёт коннектор и инициализирует REST-менеджер.
 *
 * WebSocket-сокеты здесь НЕ создаются — они живут внутри
 * WebSocketWorker'ов, которыми управляют пулы (m_wsPools).
 * Пулы создаются лениво при первой подписке.
 */
BybitConnector::BybitConnector(QObject* parent)
	: IExchangeConnector(parent)
	, m_manager(new QNetworkAccessManager)
{
	qDebug() << "[BybitConnector] Initialized. REST manager ready.";
}

//connect / disconnect
/**
 * @brief Проверяет доступность Bybit через REST ping (/v5/market/time).
 *
 * Асинхронный GET-запрос. При успехе onPingFinished() запустит
 * загрузку списка символов. Это лёгкая проверка связи перед началом
 * работы — сам WebSocket открывается позже, при первой подписке.
 */
void BybitConnector::connect()
{
	qDebug() << "[BybitConnectr] initiating connection to Bybbit API v5...";
	emit EventBus::instance().networkStatusChanged("Bybit", "Connecting ...");

	QNetworkRequest request(QUrl(QString(BybitConfig::REST_BASE_URL) + BybitConfig::REST_PING));

	QNetworkReply* reply = m_manager->get(request);

	QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]()
		{
			onPingFinished(reply);
		});
}

/**
 * @brief Закрывает все WebSocket-пулы и их воркеры.
 *
 * Каждый пул удаляется через deleteLater() — воркеры внутри
 * сами закроют свои сокеты в деструкторе. REST-менеджер
 * остаётся жив (он принадлежит this через parent).
 */
void BybitConnector::disconnect()
{
	qDebug() << "[BybitConnector] Disconnecting...";
	
	for (WebSocketPool* pool : m_wsPools)
		pool->deleteLater();
	m_wsPools.clear();
	emit EventBus::instance().networkStatusChanged("Bybit", "Disconnected");
}
// subscribeQuotes / unsubscribeQuotes
/**
 * @brief Подписывается на live-поток данных для символа из ctx.
 *
 * Строит топик и payload по типу стрима (Kline/OrderBook/Trades),
 * затем передаёт их в нужный пул. Пул сам выберет свободный воркер
 * или создаст новый. Несколько графиков подписываются параллельно
 * без закрытия чужих сокетов — это решает проблему с пропаданием
 * котировок при открытии второго окна.
 */
void BybitConnector::subscribeQuotes(const MarketContext& ctx)
{
	QString topic = buildTopic(ctx);
	QString payload = buildPayload(ctx);

	if (topic.isEmpty())
	{
		qWarning() << "[BybitConnector] Cannot build topic for:" << ctx.symbol;
		return;
	}
	WebSocketPool* pool = getOrCreatePool(ctx);
	pool->subscribe(topic, payload);

	qDebug() << "[BybitConnector] Subscribe:" << topic
		<< "→ Pool:" << pool->poolId()
		<< "| Workers:" << pool->workerCount()
		<< "| Topics:" << pool->topicCount();
}

void BybitConnector::unsubcribeQuotes(const MarketContext& ctx)
{
	QString wsUrl = getWsEndpoint(ctx.marketType);
	QString poolKey = wsUrl + "|" + streamTypeToString(ctx.streamType);
	if (!m_wsPools.contains(poolKey))
	{
		qDebug() << "[BybitConnector] No pool for:" << poolKey << "— skip.";
		return;
	}
	QString topic = buildTopic(ctx);
	QString payload = buildUnsubscribePayload(ctx);
	m_wsPools[poolKey]->unsubscribe(topic, payload);

	qDebug() << "[BybitConnector] Unsubscribe:" << topic << "from pool:" << poolKey;
}

//REST
void BybitConnector::onPingFinished(QNetworkReply* reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{
		QString errorMsg;
		int retCode = ByBitParser::parserPingResponse(reply->readAll(), errorMsg);
		if (retCode == 0)
		{
			qDebug() << "[BybitConnector] SUCCESS! Connection established.";
			emit EventBus::instance().networkStatusChanged("Bybit", "Connected");
			fetchAvailableSymbols();
		}
		else
		{
			qDebug() << "[BybitConnector] API REJECTED! Reason:" << errorMsg;
			emit EventBus::instance().networkStatusChanged("Bybit", "Error: " + errorMsg);
		}
	}
	else
	{
		qDebug() << "[BybitConnector] NETWORK ERROR:" << reply->errorString();
		emit EventBus::instance().networkStatusChanged("Bybit", "Network Error");
	}
	reply->deleteLater();
}
/**
 * @brief Асинхронно загружает исторические свечи (Kline) для заданного контекста.
 *
 * @param ctx Контекст запроса (биржевой символ, таймфрейм, лимит, конечное время и т.д.)
 *
 * Логика работы:
 * 1. Формируется уникальный requestKey = "chartId_symbol_marketType".
 * 2. Защита от дублей:
 *    - Если запрос с таким ключом уже есть в m_activeHistoryReplies и он ещё выполняется,
 *      то старый запрос прерывается (abort), а новый будет отправлен.
 * 3. Строятся параметры запроса (category, symbol, interval, limit, end).
 * 4. Отправляется GET-запрос к REST API Bybit (endpoint /v5/market/kline).
 * 5. Запрос сохраняется в m_activeHistoryReplies под ключом requestKey.
 * 6. В обработчике finished:
 *    - Удаляем запись из m_activeHistoryReplies.
 *    - Если запрос был отменён (OperationCanceledError) — просто выходим.
 *    - Если ошибка — шлём сигнал historyLoadFailed.
 *    - Если успех — парсим свечи и шлём сигнал historyChunkLoaded.
 *
 * @note Обработчик завершения использует QScopedPointer с deleteLater() для автоматического
 *       удаления QNetworkReply при выходе из лямбды.
 * @note Сигналы historyChunkLoaded / historyLoadFailed идут в соответствующий график (chartId).
 */
void BybitConnector::fetchHistory(const MarketContext& ctx)
{
	QString targetSymbol = ctx.symbol;
	int chartId = ctx.chartId;
	QString requestKey = QString::number(chartId) + "_" + targetSymbol + "_" + ctx.marketType;

	//ЗАЩИТА ОТ ДУБЛЕЙ!!!
	if (m_activeHistoryReplies.contains(requestKey))
	{
		QPointer<QNetworkReply> oldReply = m_activeHistoryReplies.value(requestKey);
		if (oldReply && oldReply->isRunning())
		{
			qWarning() << "[Bybit REST] Aborting duplicate request for:" << requestKey;
			oldReply->abort();
		}
	}
	QUrlQuery query;
	query.addQueryItem("category", getRestCategory(ctx.marketType));
	query.addQueryItem("symbol", ctx.symbol);
	query.addQueryItem("interval", intervalToByBitString(ctx.interval));
	query.addQueryItem("limit", QString::number(ctx.limit));

	if (ctx.endTime > 0)
	{
		query.addQueryItem("end", QString::number(ctx.endTime));
	}

	QUrl url(QString(BybitConfig::REST_BASE_URL) + BybitConfig::REST_KLINE);
	url.setQuery(query);

	QNetworkRequest request(url);
	request.setTransferTimeout(BybitConfig::REST_HISTORY_TIMEOUT_MS); //Запрсо на сервер если 12 сек нет ответа будет ошибка QNetworkReply::TimeoutError!!!ошибка
	qDebug() << "[Bybit REST] Sending history request. Key:" << requestKey
		<< "Limit:" << ctx.limit;

	QNetworkReply* reply = m_manager->get(request);
	QPointer<BybitConnector>self(this);
	m_activeHistoryReplies[requestKey] = reply;
	
	QObject::connect(reply, &QNetworkReply::finished, this, [self, reply, targetSymbol, requestKey, chartId]()
		{
			QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> guard(reply);
			if (!self) return;

			self->m_activeHistoryReplies.remove(requestKey);
			if (reply->error() == QNetworkReply::OperationCanceledError)
			{
				qDebug() << "[Bybit REST] Request canceled for chart:" << chartId;
				return;
			}
			if (reply->error() != QNetworkReply::NoError)
			{
				qWarning() << "[Bybit REST] Error for:" << targetSymbol
					<< reply->errorString();
				emit self->historyLoadFailed(chartId, targetSymbol, reply->errorString());
				return;
			}
			std::vector<Candle> chunk = ByBitParser::parseHistory(reply->readAll());
			emit self->historyChunkLoaded(chartId, targetSymbol, chunk);
		});
}
/**
 * @brief Асинхронно загружает списки доступных символов (торговых пар) для всех категорий.
 *
 * Поскольку Bybit требует отдельный запрос для каждой категории (spot, linear, inverse, option),
 * эта функция запускает параллельные запросы (количество = SYMBOL_CATEGORIES.size()).
 *
 * Механизм работы:
 * 1. Создаётся структура SyncContext, в которой хранятся:
 *    - accumulateSymbols: общий список накопленных пар (символ + тип рынка)
 *    - pendingRequests: счётчик оставшихся запросов (инициализируется числом категорий)
 * 2. Для каждой категории запускается лямбда launchRequest, которая:
 *    - Формирует GET-запрос к REST API (метод /v5/market/instruments-info)
 *    - Устанавливает таймаут REST_SYMBOLS_TIMEOUT_MS
 *    - Отправляет запрос и через QPointer защищает доступ к this
 * 3. В обработчике finished каждого запроса:
 *    - Если ошибок нет, парсим JSON в список символов (ByBitParser::parseAvailableSymbols)
 *    - Преобразуем категорию (например "linear") в UI-тип ("PERP") через getUiMarketType
 *    - Добавляем пары {symbol, marketType} в общий syncCtx->accumulateSymbols
 *    - Уменьшаем счётчик pendingRequests
 * 4. Когда последний запрос завершился (pendingRequests == 0):
 *    - Сортируем все накопленные пары по имени символа
 *    - Излучаем один сигнал availableSymbolsLoaded с именем биржи "Bybit" и готовым списком
 *      (сигнал идёт в EventBus, далее MarketDataManager кэширует эти данные)
 *
 * Защита от дублей: явно не требуется, т.к. функция вызывается редко (при старте или по запросу),
 * но если вызвать повторно до завершения предыдущих запросов, то старые ответы будут
 * дописываться в тот же accumulateSymbols, а после последнего запроса сортировка и сигнал
 * перезапишут кэш новым списком. Это допустимое поведение.
 *
 * @note Используется shared_ptr для SyncContext, чтобы объект жил до обработки всех ответов.
 * @note Каждый ответ удаляется через QScopedPointer с deleteLater().
 */
void BybitConnector::fetchAvailableSymbols()
{
	struct SyncContext
	{
		QList<std::pair<QString, QString>> accumulateSymbols;
		int pendingRequests = 4;
	};
	std::shared_ptr<SyncContext> syncCtx = std::make_shared<SyncContext>();

	std::function<void(const QString&)> launchRequest = [this, syncCtx](const QString& category)
	{
		QUrl url(QString(BybitConfig::REST_BASE_URL) + BybitConfig::REST_INSTRUMENTS);
		QUrlQuery query;
		query.addQueryItem("category", category);
		url.setQuery(query);
		
		QNetworkRequest request(url);
		request.setTransferTimeout(BybitConfig::REST_SYMBOLS_TIMEOUT_MS);//Запрсо на сервер если 12 сек нет ответа будет ошибка QNetworkReply::TimeoutError!!!

		QNetworkReply* reply = m_manager->get(request);
		QPointer<BybitConnector> self(this);//создает указатель на себя!! для проверки 
		
		QObject::connect(reply, &QNetworkReply::finished, this, [self, reply, syncCtx, category]()
			{
			QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> guard(reply);// Умный указатель котоырй вторым Шаблонным типом берет функтор котоырй вызывает deleteLater к reply когда произойдет выход из области видимости
			if (!self) return;
			if (reply->error() == QNetworkReply::NoError)
			{
				QStringList parsed = ByBitParser::parseAvailableSymbols(reply->readAll());
				QString marketType = self->getUiMarketType(category);
				for (const QString& symbol : parsed)
				{
					syncCtx->accumulateSymbols.append({ symbol, marketType });
				}
			}
			syncCtx->pendingRequests--;
			if (syncCtx->pendingRequests == 0)
			{
				std::sort(syncCtx->accumulateSymbols.begin(), syncCtx->accumulateSymbols.end());
				emit EventBus::instance().availableSymbolsLoaded("Bybit", syncCtx->accumulateSymbols);
			}
			});
	};
	for (const QString& category : BybitConfig::SYMBOL_CATEGORIES)
		launchRequest(category);

}
// WS - единый обработчик
/**
 * @brief Обработчик входящих сообщений от WebSocket-сервера Bybit.
 *
 * Вызывается автоматически при получении нового текстового сообщения (JSON).
 * Основная задача: распарсить сообщение и, если оно содержит обновление свечи (Kline),
 * извлечь из него символ, интервал и данные свечи, после чего уведомить систему через EventBus.
 *
 * @param message Сырое текстовое сообщение, полученное от WebSocket (обычно в формате JSON).
 *
 * @details
 * 1. Создаются локальные переменные `symbol` и `interval`, которые будут заполнены парсером.
 * 2. Вызывается `ByBitParser::parseLiveCandle()`, которая:
 *    - Проверяет, является ли сообщение свечным (по наличию поля "topic" с префиксом "kline").
 *    - Извлекает из поля `topic` интервал (например, "5" -> 5 минут) и символ.
 *    - Парсит массив `data` и заполняет структуру `Candle` (open, high, low, close, volume, timestamp).
 *    - Возвращает `std::optional<Candle>`: содержит свечу, если парсинг успешен, иначе `std::nullopt`.
 * 3. Если парсинг успешен (`liveCandle.has_value()` == true):
 *    - Генерируется сигнал `liveCandleReceived` через глобальную шину событий `EventBus`.
 *      В сигнал передаются: биржа ("Bybit"), символ, интервал и сама свеча.
 *    - В отладочных целях в консоль выводится символ и цена закрытия свечи.
 *
 * @note Сообщения, не являющиеся свечными (например, ответы на подписку, пинг-понг, ошибки),
 *       игнорируются – парсер возвращает `std::nullopt`, и функция ничего не делает.
 * @see ByBitParser::parseLiveCandle
 * @see EventBus::liveCandleReceived
 */
void BybitConnector::onWsMessageReceived(const QString& message)
{
	QByteArray raw = message.toUtf8();

	// Kline
	if (message.contains("\"kline."))
	{
		QString symbol;
		ChartInterval interval;
		std::optional<Candle> liveCandle =
			ByBitParser::parseLiveCandle(raw, symbol, interval);
		if (liveCandle.has_value())
		{
			emit EventBus::instance().liveCandleReceived("Bybit", symbol, interval, liveCandle.value());
			qDebug() << "[LIVE]" << symbol << "Price:" << liveCandle->close;
		}
		return;
	}

	// OrderBook
	if (message.contains("\"orderbook."))
	{
		QString symbol;
		bool    isDelta = false;
		std::optional<OrderBookSnapshot> snap =
			ByBitParser::parseOrderBook(raw, symbol, isDelta);
		if (snap.has_value())
		{
			emit EventBus::instance().orderBookReceived("Bybit", symbol, snap.value(), isDelta);
			qDebug() << "[ORDERBOOK]" << symbol
				<< (isDelta ? "delta" : "snapshot")
				<< "asks:" << snap->asks.size()
				<< "bids:" << snap->bids.size();
		}
		return;
	}
}

//private helpers - WebSocket
/**
 * @brief Возвращает существующий WebSocketPool для заданного контекста или создаёт новый.
 *
 * Каждый пул идентифицируется уникальным ключом, который объединяет:
 * - WebSocket URL (зависит от marketType, например "wss://.../spot" или ".../linear")
 * - Тип потока (Kline / OrderBook / Trades)
 *
 * Это гарантирует, что свечи, стаканы и сделки даже для одного символа будут
 * идти через разные пулы (и разные WebSocket-соединения), что позволяет изолировать
 * нагрузку (тяжёлый стакан не тормозит свечи).
 *
 * @param ctx Контекст запроса (marketType, streamType и др.)
 * @return Указатель на WebSocketPool (всегда валидный, создан или получен из кэша)
 *
 * @details
 * 1. Получаем WebSocket URL для данного типа рынка (getWsEndpoint).
 * 2. Формируем poolKey = "wsUrl|streamType" (например "wss://.../linear|Kline").
 * 3. Если пула с таким ключом ещё нет:
 *    - Создаём новый WebSocketPool.
 *    - Подключаем его сигнал messageReceived к нашему слоту onWsMessageReceived.
 *    - Сохраняем пул в хеш-таблицу m_wsPools.
 * 4. Возвращаем пул (новый или существующий).
 *
 * @note Пул не удаляется самостоятельно, он живёт пока жив BybitConnector (parent = this).
 * @see getWsEndpoint
 * @see streamTypeToString
 */
WebSocketPool* BybitConnector::getOrCreatePool(const MarketContext& ctx)
{
	QString wsUrl = getWsEndpoint(ctx.marketType);
	QString poolKey = wsUrl + "|" + streamTypeToString(ctx.streamType);

	if (!m_wsPools.contains(poolKey))
	{
		QString poolId = ctx.marketType + "|" + streamTypeToString(ctx.streamType);
		WebSocketPool* pool = new WebSocketPool(wsUrl, poolId, this);
		
		QObject::connect(pool, &WebSocketPool::messageReceived, this, &BybitConnector::onWsMessageReceived);

		m_wsPools.insert(poolKey, pool);
		qDebug() << "[BybitConnector] Created pool:" << poolId;
	}
	return m_wsPools[poolKey];
}

QString BybitConnector::buildTopic(const MarketContext& ctx) const
{
	switch (ctx.streamType)
	{
	case StreamType::Kline:
		return QString("kline.%1.%2")
			.arg(intervalToByBitString(ctx.interval), ctx.symbol);
	case StreamType::OrderBook:
		return QString("orderbook.%1.%2")
			.arg(BybitConfig::ORDERBOOK_DEPTH).arg(ctx.symbol);
	case StreamType::Trades:
		return QString("publicTrade.%1").arg(ctx.symbol);
	default:
		qWarning() << "[BybitConnector] Unknown StreamType!";
		return {};
	}
}

QString BybitConnector::buildPayload(const MarketContext& ctx) const
{

	return ByBitParser::buildSubscriptionRequest(buildTopic(ctx), ctx.symbol);
}

QString BybitConnector::buildUnsubscribePayload(const MarketContext& ctx) const
{
	return ByBitParser::buildUnsubscribeRequest(buildTopic(ctx), ctx.symbol);
}

//private helpers - converters
QString BybitConnector::streamTypeToString(StreamType type) const
{
	switch (type)
	{
	case StreamType::Kline:     return "Kline";
	case StreamType::OrderBook: return "OrderBook";
	case StreamType::Trades:    return "Trades";
	default:                    return "Unknown";
	}
}

QString BybitConnector::intervalToByBitString(const ChartInterval& interval) const
{
	switch (interval.unit)
	{
	case ChartInterval::Unit::Minute: return QString::number(interval.count);
	case ChartInterval::Unit::Hour:   return QString::number(interval.count * 60);
	case ChartInterval::Unit::Day:    return "D";
	case ChartInterval::Unit::Week:   return "W";
	case ChartInterval::Unit::Month:  return "M";
	default:                          return "1";
	}

}
QString BybitConnector::getUiMarketType(const QString& category) const
{
	for (std::pair<QString, BybitConfig::MarketTypeInfo> pair : BybitConfig::MARKET_TYPE_MAP.asKeyValueRange())
	{
		const QString& key = pair.first;
		const BybitConfig::MarketTypeInfo& info = pair.second;
		
		if (QString(info.restCategory) == category)
			return key;
	}
	return "PERP";

}

/**
 * @brief Возвращает строку category для REST-запроса по типу рынка.
 *
 * По ключу uiMarketType (например "PERP", "SPOT") ищет в таблице MARKET_TYPE_MAP
 * соответствующую структуру MarketTypeInfo и возвращает её поле restCategory
 * (например "linear" для "PERP", "spot" для "SPOT").
 * Если ключ не найден, возвращает restCategory из DEFAULT_MARKET ("linear").
 *
 * @param uiMarketType Тип рынка из UI ("PERP", "SPOT", "INV_PERP", "OPTION")
 * @return Строка для параметра category API Bybit ("linear", "spot", "inverse", "option")
 */
QString BybitConnector::getRestCategory(const QString& uiMarketType) const
{
	return BybitConfig::MARKET_TYPE_MAP
		.value(uiMarketType, BybitConfig::DEFAULT_MARKET)
		.restCategory;
}

QString BybitConnector::getWsEndpoint(const QString& uiMarketType) const
{
	return BybitConfig::MARKET_TYPE_MAP
		.value(uiMarketType, BybitConfig::DEFAULT_MARKET)
		.wsEndpoint;
}
