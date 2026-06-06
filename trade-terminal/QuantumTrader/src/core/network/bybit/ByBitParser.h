#pragma once

#include<QByteArray>
#include<QString>
#include<QStringList>
#include<vector>
#include<optional>
#include"../common/NetworkTypes.h"
#include"../../models/Candle.h"


/**
 * @brief ByBitParser — статический парсер данных биржи Bybit.
 *
 * Все методы статические — класс не хранит состояния.
 * Разделён на три группы:
 *  - REST: парсинг ответов от HTTP API
 *  - WS входящие: парсинг сообщений от WebSocket
 *  - WS исходящие: построение JSON-сообщений для биржи
 *  - Конвертеры: вспомогательные преобразования типов
 */
class ByBitParser
{
public:
    // == REST ============================================================================

    /// Парсит ответ на ping-запрос (/v5/market/time).
    /// @return retCode (0 = успех, иное = ошибка API, -1 = ошибка парсинга)
    static int parserPingResponse(const QByteArray& rawData, QString& outErrorMsg);

    /// Парсит исторические свечи из ответа /v5/market/kline.
    /// Bybit отдает свечи в обратном порядке - метод разворачивает их.
    /// @return Свечи в хронологическом порядке(старые первые).
    static std::vector<Candle> parseHistory(const QByteArray& rawData);
	
    ///Парсит список символов из /v5/market/instruments-info
    ///@return Отсортированный список символов {"BTCUSTD", "ETHUSDT" ...}
    static QStringList parseAvailableSymbols(const QByteArray& rawData);
    

	//== WebSocket входящие сообщения =====================================================

	/// Парсит live-свечу из WS-сообщения.
    /// Извлекает символ и интервал из топика ("kline.5.BTCUSDT").
    /// Системные сообщения (pong, subscribe) возвращают nullopt без ошибок.
    /// @param outSymbol   Выходной параметр: символ ("BTCUSDT")
    /// @param outInterval Выходной параметр: интервал свечи
    static std::optional<Candle> parseLiveCandle(const QByteArray& jsonMessage, QString& outSymbol, ChartInterval& outInterval);

    //== WebSocket исходящие сообщения ====================================================
	/// Строит subscribe JSON для отправки на биржу исходя из аргументов
	/// @param topic Готовый топик, например "kline.5.BTCUSDT"
	/// @param reqId Уникальный символ
    static QString buildSubscriptionRequest(const QString& topic,
        const QString& reqId);

    /// Строит unsubscribe JSON для отправки на биржу.
    /// @param topic Топик от которого отписываемся
    /// @param reqId Уникальный id запроса (обычно symbol)
    static QString buildUnsubscribeRequest(const QString& topic,
        const QString& reqId);

    /// Строит ping JSON. req_id уникален (текущее время в мс).
    static QString buildPingRequest();

    // == Конвертеры ======================================================================
    /// Конвертирует строку интервала Bybit в ChartInterval.
    /// "5"→Minute_5, "60"→Hour_1, "240"→Hour_4, "D"→Day_1 и т.д.
    static ChartInterval intervalFromBybitString(const QString& str);

};
