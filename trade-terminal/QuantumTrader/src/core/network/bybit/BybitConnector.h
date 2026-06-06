#pragma once

#include"../common/IExchangeConnector.h"
#include<QPointer>
#include<QHash>
#include"WebSocketPool.h"
class QNetworkAccessManager;
class QNetworkReply;

/**
 * @brief BybitConnector — коннектор к бирже Bybit.
 *
 * ## Обязанности
 *  - REST: пинг, загрузка истории свечей, загрузка списка символов.
 *  - WS:   управление пулами WebSocket-подписок через WebSocketPool.
 *
 * ## Архитектура WebSocket
 *  Коннектор НЕ держит сокет напрямую. Вместо этого управляет
 *  пулами воркеров — по одному пулу на каждый тип данных:
 *
 *  @code
 *  BybitConnector	/// Корче генерирует запрос 

 *      ├── pool["wss://linear|Kline"]      ← свечи linear
 *      ├── pool["wss://spot|Kline"]        ← свечи spot
 *      ├── pool["wss://linear|OrderBook"]  ← стаканы (в будущем)
 *      └── pool["wss://linear|Trades"]     ← лента сделок (в будущем)
 *  @endcode
 *
 * ## REST и WS полностью разделены
 *  REST (QNetworkAccessManager) — только история и символы.
 *  WS   (WebSocketPool)         — только live-данные.
 */
class BybitConnector : public IExchangeConnector
{
	Q_OBJECT
public:
	explicit BybitConnector(QObject* parent = nullptr);

	/// Проверяет доступность Bybit через REST ping, затем грузит символы.
	void connect()override;

	/// Закрывает все WebSocket-пулы (воркеры сами закроют свои сокеты).
	void disconnect()override;
	/// Загружает исторические свечи через REST. Дубли по chartId отменяются.
	void fetchHistory(const MarketContext& ctx)override;
	void subscribeQuotes(const MarketContext& ctx)override;
	void unsubcribeQuotes(const MarketContext& ctx) override;

private slots:
	void onPingFinished(QNetworkReply* reply);
	void onWsMessageReceived	/// Корче генерирует запрос 
(const QString& message);
private:
	//REST
	/// Параллельно грузит символы 4 категорий, отдаёт одним сигналом.
	void fetchAvailableSymbols();
	//WS helpers
	WebSocketPool* getOrCreatePool(const MarketContext& ctx);
	/// Строит топик Bybit: "kline.5.BTCUSDT", "orderbook.50.BTCUSDT" и т.д.
	QString buildTopic(const MarketContext& ctx)const;
	QString buildPayload(const MarketContext& ctx)const;
	QString buildUnsubscribePayload(const MarketContext& ctx)const;
	//Converters
	QString intervalToByBitString(const ChartInterval& interval)const;
	QString getRestCategory(const QString& uiMarketType)const;
	QString getWsEndpoint(const QString& uiMarketType)const;

	/// REST-категория ("spot") → UI-тип ("SPOT"). Обратный поиск по таблице.
	QString getUiMarketType(const QString& category)const;
	QString streamTypeToString(StreamType type)const;
	//Members
	QNetworkAccessManager* m_manager = nullptr;
	//WebSocket worker pools: key = "wsUrl|streamType"
	QHash<QString, WebSocketPool*> m_wsPools;
	//Active REST history requests : key = "chartId_symbol_marketType"
	QHash<QString, QPointer<QNetworkReply>> m_activeHistoryReplies;
};