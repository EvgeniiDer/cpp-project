#pragma once

#include<QObject>
#include<QHash>
#include<QList>
#include"WebSocketWorker.h"

/**
 * @brief WebSocketPool — диспетчер пула WebSocketWorker'ов
 *        для одного эндпоинта и одного типа данных.
 *
 * ## Концепция
 *  Биржа Bybit ограничивает количество подписок на один WebSocket — не более 20.
 *  Если открыть много окон (графики, стаканы, лента сделок), одного сокета
 *  не хватит. WebSocketPool решает эту проблему автоматически:
 *
 *  - Держит список WebSocketWorker'ов (каждый = один физический сокет).
 *  - При запросе подписки находит воркер со свободным местом.
 *  - Если все воркеры заполнены — создаёт новый физический сокет.
 *  - Все входящие рыночные сообщения пробрасывает через messageReceived.
 *
 * ## Принцип "один пул = один тип данных"
 *  Потоки данных разных типов изолированы по разным пулам:
 *
 *  @code
 *  BybitConnector
 *      ├── pool["linear|Kline"]      ← только свечи
 *      │       Worker_1 [до 20 подписок на свечи]
 *      │       Worker_2 [создаётся автоматически при переполнении]
 *      ├── pool["linear|OrderBook"]  ← только стаканы (тяжёлые, 100-200мс)
 *      │       Worker_1 [стаканы изолированы от свечей]
 *      └── pool["linear|Trades"]     ← лента сделок
 *              Worker_1
 *  @endcode
 *
 *  Изоляция важна: поток стакана очень плотный (обновления каждые 100-200мс).
 *  Если смешать его со свечами в одном сокете — буфер ОС начнёт захлёбываться
 *  и котировки получат задержки, что критично для трейдинга.
 *
 * ## Маршрутизация подписок
 *  Пул хранит карту topic → workerId, чтобы при unsubscribe точно знать
 *  в каком воркере находится нужная подписка.
 *
 * ## Расширяемость
 *  Один и тот же WebSocketPool используется для любой биржи — Bybit, Alor
 *  и других. Коннектор каждой биржи создаёт свои пулы с нужными URL'ами.
 *  Виджеты (стакан, график) работают через MarketDataManager и не знают
 *  о существовании ни пулов, ни воркеров.
 *
 * ## Жизненный цикл подписки
 *  @code
 *  subscribe(topic, payload)
 *      → если topic уже есть — выходим (дубликаты не допускаются)
 *      → findOrCreateWorker() — ищем воркер с местом или создаём новый
 *      → worker->subscribe(topic, payload)
 *      → запоминаем topic → workerId в m_topicWorkerMap
 *
 *  unsubscribe(topic, payload)
 *      → находим workerId по topic из m_topicWorkerMap
 *      → удаляем topic из m_topicWorkerMap
 *      → worker->unsubscribe(topic, payload)
 *  @endcode
 */

class WebSocketPool : public QObject
{
	Q_OBJECT
public:
	explicit WebSocketPool(const QString& wsUrl, const QString& poolId, QObject* parent = nullptr);
	~WebSocketPool()override;
	
	void subscribe(const QString& topic, const QString& jsonPayload);
	void unsubscribe(const QString& topic, const QString& jsonPayload);

	[[nodiscard]] QString poolId()const { return m_poolId;}
	[[nodiscard]] QString wsUrl()const { return m_wsUrl; }
	[[nodiscard]] int workerCount()const { return m_workers.size(); }
	[[nodiscard]] int topicCount()const { return m_topicWorkerMap.size(); }
	[[nodiscard]] bool hasTopic(const QString& topic) const { return m_topicWorkerMap.contains(topic); }
	signals:
		void messageReceived(const QString& message);
private slots:
	void onWorkerMessage(const QString& workerId, const QString& message);
	void onWorkerStatus(const QString& workerId, const QString& status);
private:
	struct TopicEntry
	{
		QString workerId;
		int refCount = 1;
	};
	WebSocketWorker* findOrCreateWorker();

	QString m_wsUrl;
	QString m_poolId;
	QList<WebSocketWorker*> m_workers;

	QHash<QString, TopicEntry> m_topicWorkerMap;
};
