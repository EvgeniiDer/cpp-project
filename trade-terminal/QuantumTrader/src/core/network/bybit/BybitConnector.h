#pragma once

#include"../common/IExchangeConnector.h"
#include<QObject>
#include<QString>
#include<QPointer>
#include<vector>

class QNetworkAccessManager;
class QNetworkReply;
class QWebSocket;
class QTimer;


class BybitConnector : public IExchangeConnector
{
	Q_OBJECT
public:
	explicit BybitConnector(QObject* parent = nullptr);
	~BybitConnector() override = default;
	void connect()override;
	void disconnect()override;
	/**
	 * 📥 Асинхронно запрашивает один чанк (кусок) исторических свечей с биржи Bybit.
	 *
	 * Метод отправляет HTTP GET-запрос к эндпоинту /v5/market/kline.
	 * Ответ приходит асинхронно – после получения данных испускается сигнал
	 * historyChunkLoaded(symbol, chunk) или historyLoadFailed(symbol, error).
	 *
	 * 🔄 Что такое «чанк»:
	 *   - Это одна порция свечей, которую биржа отдаёт за один запрос.
	 *   - Максимальный размер чанка обычно 1000 свечей (ограничение Bybit).
	 *   - Параметр `limit` может быть больше, но сервер всё равно вернёт не более лимита биржи.
	 *
	 * 🧩 Параметры:
	 *   @param symbol      Торговый символ (например, "BTCUSDT").
	 *   @param marketType  Тип рынка ("spot", "linear", "inverse", "option") – определяет категорию в API.
	 *   @param interval    Таймфрейм свечи (1m, 5m, 1h и т.д.).
	 *   @param limit       Желаемое количество свечей в этом чанке (обычно 1000 или меньше).
	 *   @param endTime     Временная метка в миллисекундах (Unix time).
	 *                      Если > 0, сервер вернёт свечи **строго старее** этого времени.
	 *                      Используется для пагинации (загрузка следующих кусков в прошлое).
	 *
	 * 🚦 Сигналы, которые могут быть испущены:
	 *   - historyChunkLoaded(symbol, chunk)  – при успешном получении и парсинге.
	 *   - historyLoadFailed(symbol, error)   – при ошибке сети, таймауте или проблеме с API.
	 *
	 * ⚠️ Важные замечания:
	 *   - Метод асинхронный, не блокирует поток. Результат придёт позже через сигнал.
	 *   - Чанки могут возвращаться в любом порядке, если делать несколько параллельных запросов.
	 *   - Для последовательной загрузки большой истории нужно вызывать этот метод повторно,
	 *     каждый раз сдвигая `endTime` в прошлое (например, на основе самой старой свечи предыдущего чанка).
	 *
	 * @see CandleHistoryManager::onChunkLoaded – пример обработки полученных чанков.
	 * @see ByBitParser::parseHistory       – парсинг сырого JSON в вектор Candle.
	 */
	void fetchHistory(const MarketContext& ctx)override;
	void subscribeQuotes(const QString& symbol, const QString& marketType)override;
	/**
	 * Асинхронно загружает список всех торговых инструментов (символов)
	 * со всех 4 категорий Bybit: spot, linear, inverse, option.
	 *
	 * ⚙️ Модель выполнения: АСИНХРОННАЯ КОНКУРЕНТНОСТЬ (без параллелизма).
	 *    - Все запросы выполняются в одном потоке (главном event loop'е Qt).
	 *    - Нет создания дополнительных потоков – это НЕ параллелизм.
	 *    - QNetworkAccessManager использует неблокирующие сокеты, что позволяет
	 *      конкурентно ожидать ответы от сервера, не блокируя поток.
	 *
	 * 🔄 Алгоритм:
	 *   1. Создаёт контекст SyncContext (shared_ptr) для сбора результатов.
	 *   2. Запускает 4 GET-запроса (через лямбду launchCategoryRequest) без ожидания.
	 *   3. Каждый ответ обрабатывается в отдельном слоте finished:
	 *      - При успехе парсит JSON -> список символов + тип рынка.
	 *      - Добавляет пары (символ, тип) в общий список.
	 *   4. Когда все 4 запроса завершены (pendingRequests == 0):
	 *      - Сортирует список по символам.
	 *      - Испускает сигнал availableSymbolsLoaded() через EventBus.
	 *
	 * ⚠️ Важно: метод возвращает управление сразу после запуска запросов.
	 *    Результат будет получен позже через сигнал.
	 *
	 * @see EventBus::availableSymbolsLoaded
	 */
	void fetchAvailableSymbols();

private slots:
	void onPingFinished(QNetworkReply* reply);
	void onWsConnected();
	void onWsDisconected();
	void onWsTextMessageReceived(const QString& message);
	void sendWsPing();
private:
	QWebSocket* m_webSocket = nullptr;
	QNetworkAccessManager* m_manager = nullptr;
	QTimer* m_pingTimer = nullptr;

	QString intervalToByBitString(const ChartInterval& interval)const;
	QString getRestCategory(const QString& uiMarketType)const;
	QString getWsEndpoint(const QString& uiMarketType)const;
	QString getUiMarketType(const QString& category)const;

	QString m_currentSymbol;
	QString m_currentInterval;
	QString m_currentMarketType;
};