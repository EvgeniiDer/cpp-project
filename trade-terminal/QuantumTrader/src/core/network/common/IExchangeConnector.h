#pragma once
#include"NetworkTypes.h"
#include<QObject>
#include<QString>
#include<vector>

/**
 * @brief IExchangeConnector — абстрактный интерфейс коннектора к бирже.
 *
 * Каждая биржа (Bybit, Alor, Binance...) реализует этот интерфейс.
 * MarketDataManager работает только через него и не знает о конкретных
 * реализациях — это позволяет добавлять новые биржи без изменения
 * остального кода.
 *
 * ## Порядок вызовов
 * @code
 * connector->connect()                    // 1. REST ping + загрузка символов
 * connector->fetchHistory(ctx)            // 2. Загрузка истории свечей
 * connector->subscribeQuotes(ctx)         // 3. Подписка на live-поток
 * connector->unsubscribeQuotes(ctx)       // 4. Отписка (смена биржи/закрытие окна)
 * connector->disconnect()                 // 5. Закрыть всё
 * @endcode
 */
class IExchangeConnector : public QObject
{
	Q_OBJECT
public:
	explicit IExchangeConnector(QObject* parent) : QObject(parent){}
	///Установить соедениен с биржей (Rest ping + загрузка символов)
	virtual void connect() = 0;
	///Закрыть все соединения(REST + WebSocket пулы)
	virtual void disconnect() = 0;
	
	/// Загрузить исторические свечи через REST API.
	virtual void fetchHistory(const MarketContext& ctx) = 0;

	/// Подписаться на live-поток данных через WebSocket пул.
	/// Повторный вызов с тем же топиком безопасен — дубликаты игнорируются.
	virtual void subscribeQuotes(const MarketContext& ctx) = 0;

	/// Отписаться от live-потока данных.
	/// Вызывается при смене биржи в группе или закрытии окна.
	virtual void unsubcribeQuotes(const MarketContext& ctx) = 0;
signals:
	///Порция исторических свечей  загружена и готова к отображению
	void historyChunkLoaded(int chartId, const QString& symbol, const std::vector<Candle>& candles);
	/// Загрузка истории завершилась ошибкой.
	void historyLoadFailed(int chartId, const QString& symbol, const QString& errorStr);

	/// Общая ошибка коннектора
	void errorOccured(const QString& message);
};