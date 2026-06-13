#pragma once

#include<QObject>
#include<QString>
#include<QHash>
#include<functional>
#include<vector>

#include"../network/common/IExchangeConnector.h"
#include"../network/common/NetworkTypes.h"
#include"../models/Candle.h"

class CandleHistoryManager;
/**
 * @brief Менеджер рыночных данных.
 * Центральный класс, управляющий подключениями к биржам,
 *          фабриками коннекторов, историей свечей и подписками на WebSocket-потоки.
 *
 * Основные обязанности:
 * - Регистрация фабрик коннекторов для разных бирж.
 * - Установка соединения с биржей (создание коннектора и менеджера истории).
 * - Маршрутизация запросов исторических данных к соответствующему CandleHistoryManager.
 * - Подписка/отписка на реальные потоки (стаканы, сделки, свечи) через активный коннектор.
 * - Кэширование списка доступных инструментов для каждой биржи (получается через EventBus).
 */
class MarketDataManager : public QObject
{
	Q_OBJECT
public:
	using ConnectorFactory = std::function <IExchangeConnector* (QObject* parent)>;
	/**
	* @brief Конструктор.
	* @param parent Родительский QObject (для автоматического управления памятью).
	*
	* Подписывается на событие availableSymbolsLoaded от EventBus, чтобы кэшировать
	* списки символов для бирж.
	*/
	explicit MarketDataManager(QObject* parent = nullptr);
	~MarketDataManager();
	/**
	* @brief Зарегистрировать фабрику для создания коннектора биржи.
	* @param exchangeName Имя биржи (например, "Bybit", "Binance").
	* @param factory Функция-фабрика, принимающая QObject* parent и возвращающая IExchangeConnector*.
	*
	* @details Фабрика вызывается при connectTo() для создания экземпляра коннектора.
	* Если фабрика для указанной биржи уже зарегистрирована, повторная регистрация игнорируется.
	*/
	void registerFactory(const QString& exchangeName, ConnectorFactory factory);
	/**
	 * @brief Установить соединение с биржой.
	 * @param exchangeName Имя биржи.
	 *
	 * @details Проверяет, нет ли уже активного коннектора. Если есть — ничего не делает.
	 * Создаёт коннектор через зарегистрированную фабрику, сохраняет его в m_activeConnectors,
	 * создаёт для него CandleHistoryManager и вызывает connector->connect().
	 *
	 * @warning Если фабрика для биржи не зарегистрирована, в лог выводится критическая ошибка.
	 */
	void connectTo(const QString& exchangeName);
	/**
	 * @brief Запросить исторические свечи.
	 * @param ctx Контекст запроса (биржа, символ, таймфрейм, лимит и т.д.).
	 *
	 * @details Перенаправляет запрос в CandleHistoryManager, соответствующий бирже ctx.exchange.
	 * Если менеджер истории отсутствует (биржа не активна), выводит сообщение об ошибке.
	 */
	void requestHistory(const MarketContext& ctx);
	/**
	 * @brief Подписаться на WebSocket-поток (стакан, сделки или свечи в реальном времени).
	 * @param ctx Контекст подписки (биржа, символ, тип потока StreamType и др.).
	 *
	 * @details Вызывает метод subscribeQuotes() активного коннектора.
	 * Если коннектор для биржи не активен, выводит ошибку.
	 */
	void subscribeToStream(const MarketContext& ctx);
	/**
	* @brief Отписаться от WebSocket-потока.
	* @param ctx Контекст отписки.
	*
	* @details Вызывает unsubcribeQuotes() активного коннектора.
	*/
	void unsubscribeFromStream(const MarketContext& ctx);
	/**
	 * @brief Получить кэшированный список символов для биржи.
	 * @param exchangeName Имя биржи.
	 * @return QList<std::pair<QString, QString>> Список пар (symbol, marketType).
	 *
	 * @details Кэш заполняется асинхронно при получении сигнала availableSymbolsLoaded от EventBus.
	 * Если данные ещё не загружены, возвращает пустой список.
	 */
	QList<std::pair<QString, QString>>getCachedSymbols(const QString& exchangeName)const;
signals:
	
	void tickerUpdated(const QString& exchangeName, const QString& symbol, double lastPrice, double volume);
private:
	QHash<QString, ConnectorFactory> m_factories;
	QHash<QString, IExchangeConnector*> m_activeConnectors;

	QHash<QString, CandleHistoryManager*>m_historyManagers;

	QHash<QString, QList<std::pair<QString,QString>>> m_cachedSymbols;

};
