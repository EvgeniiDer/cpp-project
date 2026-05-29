#pragma once
#include<optional>
#include<vector>
#include<QString>
#include<QByteArray>
#include"../../models/Candle.h"
class ByBitParser
{
public:
	static int parserPingResponse(const QByteArray& rawData, QString& outErrorMsg);
	static std::vector<Candle>parseHistory(const QByteArray& rawData);
	static std::optional<Candle> parseLiveCandle(const QByteArray& jsonMessage, QString& outSymbol);
	/**
 * 🔍 Парсит JSON-ответ от Bybit API (эндпоинт /v5/market/instruments-info)
 *    и извлекает из него СПИСОК ТОРГОВЫХ СИМВОЛОВ (названий валютных пар).
 *
 * ❗ ВАЖНО: возвращает НЕ ЦЕНЫ, а именно идентификаторы инструментов.
 *           Например: "BTCUSDT", "ETHUSDT" – это символы, а не цены.
 *
 * @param rawData – сырой JSON, пришедший от сервера Bybit.
 * @return QStringList – отсортированный по алфавиту список символов,
 *                       например: ["BTCUSDT", "ETHUSDT", "SOLUSDT"].
 *                       В случае ошибки (retCode != 0) или проблем с парсингом
 *                       возвращается ПУСТОЙ список.
 *
 * @details Что происходит внутри:
 *          1. Читает поле retCode из JSON.
 *          2. Если retCode != 0 – сразу возвращает пустой список (ошибка API).
 *          3. Берёт массив result.list.
 *          4. У каждого объекта в массиве извлекает поле "symbol".
 *          5. Кладет символы в QStringList и сортирует их.
 *          6. При любых исключениях simdjson логирует ошибку в qDebug() и возвращает пустой список.
 *
 * @note Для получения реальных цен используй другой метод (например, /v5/market/tickers).
 *
 * @see Пример JSON, который парсится:
 * {
 *   "retCode": 0,
 *   "result": {
 *     "list": [
 *       { "symbol": "BTCUSDT", ... },
 *       { "symbol": "ETHUSDT", ... }
 *     ]
 *   }
 * }
 */
	static QStringList parseAvailableSymbols(const QByteArray& rawData);
	static QString buildSubscriptionRequest(const QString& symbol, const QString& interval);
	static QString buildPingRequest();
};