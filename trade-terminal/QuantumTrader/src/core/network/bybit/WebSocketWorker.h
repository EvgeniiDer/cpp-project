#pragma once

#include<QObject>
#include<QHash>

class QWebSocket;
class QTimer;
namespace Bybit
{
	constexpr int PING_INTERVAL_MS = 20000;
	constexpr int PONG_TIMEOUT_MS = 5000;
	constexpr int RECONNECT_DELAY_MS = 3000;
	constexpr int MAX_SUBSCRIPTIONS = 20;
}
/**
 * @brief WebSocketWorker — один физический WebSocket-сокет с полным
 *        управлением жизненным циклом подписок и соединения.
 *
 * ## Обязанности
 *  - Держит один QWebSocket подключённым к эндпоинту биржи.
 *  - Хранит словарь активных подписок: topic → jsonPayload.
 *  - Автоматически восстанавливает все подписки после реконнекта.
 *  - Следит за живостью соединения через механизм ping/pong.
 *
 * ## Хранилище подписок: QHash<topic, payload>
 *  - topic   — уникальный ключ подписки, например "kline.5.BTCUSDT"
 *  - payload — готовый JSON для отправки на биржу
 *
 *  Payload хранится рядом с topic специально для реконнекта:
 *  после переподключения onConnected() вызывает resubscribeAll(),
 *  который переотправляет все payload'ы на биржу без потери подписок.
 *
 * ## Механизм ping/pong (защита от зомби-соединений)
 *  Каждые 20 секунд sendPing() отправляет {"op":"ping"} и взводит
 *  двойную защиту от зомби-соединения:
 *
 *  1. m_pongWatchdog (5 сек) — одноразовый таймер. Если pong не
 *     пришёл за 5 секунд → forceReconnect().
 *
 *  2. Проверка в sendPing() — если m_waitingForPong = true в момент
 *     следующего ping (pong не пришёл за целый интервал 20 сек)
 *     → forceReconnect().
 *
 * ## Жизненный цикл подписки
 *  @code
 *  subscribe(topic, payload)
 *      → сохраняем в m_subscriptions
 *      → если подключены: отправляем payload немедленно
 *      → если нет: onConnected() → resubscribeAll() отправит позже
 *
 *  unsubscribe(topic, payload)
 *      → удаляем из m_subscriptions
 *      → если подключены: отправляем unsubscribe payload на биржу
 *
 *  forceReconnect() / scheduleReconnect()
 *      → m_subscriptions НЕ чистим
 *      → после переподключения onConnected() → resubscribeAll()
 *      → все подписки восстанавливаются автоматически
 *  @endcode
 *
 * ## Использование
 *  Воркер не используется напрямую — им управляет WebSocketPool,
 *  который следит за лимитом подписок и создаёт новые воркеры
 *  при его достижении.
 */
class WebSocketWorker : public QObject
{
	Q_OBJECT
public:
	explicit WebSocketWorker(const QString& workId, const QString& wsUrl, QObject* parent = nullptr);
	~WebSocketWorker()override;

	[[nodiscard]] QString getId()const { return m_workerId; }
	[[nodiscard]] QString getUrl()const { return m_wsUrl; }
	[[nodiscard]] bool canAccept()const { return m_subscriptions.size() < Bybit::MAX_SUBSCRIPTIONS; }
	[[nodiscard]] bool containsTopic(const QString& topic)const { return m_subscriptions.contains(topic); }
	[[nodiscard]] bool isEmpty() const { return m_subscriptions.isEmpty(); }

	void subscribe(const QString& topic, const QString& jsonPayload);
	void unsubscribe(const QString& topic, const QString& jsonPayload);

	signals:
		void messageReceived(const QString& workerId, const QString& message);
		void statusChanged(const QString& workerId, const QString& status);
private slots:
	void onConnected();
	void onDisconnected();
	void onTextMessageReceived(const QString& message);
	void sendPing();
	void onPongTimeout();
private:
	void resubscribeAll();
	void forceReconnect();
	void scheduleReconnect();

	QString m_workerId;
	QString m_wsUrl;

	QWebSocket* m_socket = nullptr;
	QTimer* m_pingTimer = nullptr;
	QTimer* m_pongWatchdog = nullptr;

	QHash<QString, QString> m_subscriptions;

	bool m_waitingForPong = false;
	bool m_isConnected = false;

};