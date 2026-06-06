#include"WebSocketPool.h"
#include<QDebug>



//constructor/destructor

WebSocketPool::WebSocketPool(const QString& wsUrl, const QString& poolId, QObject* parent)
	:QObject(parent)
	,m_wsUrl(wsUrl)
	,m_poolId(poolId)
{
	qDebug() << "[Pool]" << m_poolId << "Created for Url: " << m_wsUrl;
}

WebSocketPool::~WebSocketPool()
{
	qDebug() << "[Pool]" << m_poolId << "Destroyed." << " Workers: " << m_workers.size() << " Topics: " << m_topicWorkerMap.size();
}

void WebSocketPool::subscribe(const QString& topic, const QString& jsonPayload)
{
	if (m_topicWorkerMap.contains(topic))
	{
		m_topicWorkerMap[topic].refCount++;
		qDebug() << "[WsPool] Already subscribed to:" << topic;
		qDebug() << "[WsPool] Topic already tracked:" << topic
			<< "→ refs:" << m_topicWorkerMap[topic].refCount;
		return;
	}
	WebSocketWorker* worker = findOrCreateWorker();
	worker->subscribe(topic, jsonPayload);
	m_topicWorkerMap.insert(topic, {worker->getId(), 1});
	qDebug() << "[WsPool] Assigned topic:" << topic
		<< "→ Worker:" << worker->getId()
		<< "| Total workers:" << m_workers.size();
}

void WebSocketPool::unsubscribe(const QString& topic, const QString& jsonPayload)
{
	if (!m_topicWorkerMap.contains(topic))
	{
		qDebug() << "[WsPool] Not subscribed to:" << topic << "— skip.";
		return;
	}

	TopicEntry& entry = m_topicWorkerMap[topic];
	entry.refCount--;

	qDebug() << "[WsPool] Topic refcount--" << topic
		<< "→ refs left:" << entry.refCount;

	if (entry.refCount > 0)
	{
		qDebug() << "[WsPool] Topic still needed, keeping server subscription.";
		return;
	}

	QString workerId = entry.workerId;
	m_topicWorkerMap.remove(topic);

	for (WebSocketWorker* worker : m_workers)
	{
		if (worker->getId() == workerId)
		{
			worker->unsubscribe(topic, jsonPayload);
			break;
		}
	}

}

//Slots
void WebSocketPool::onWorkerMessage(const QString& workerId, const QString& message)
{
	emit messageReceived(message);

}

void WebSocketPool::onWorkerStatus(const QString& workerId, const QString& status)
{
	qDebug() << "[WsPool] Worker" << workerId << "→" << status;
}

//private methods
WebSocketWorker* WebSocketPool::findOrCreateWorker()
{
	//если существует
	for (WebSocketWorker* worker : m_workers)
	{
		if (worker->canAccept())
			return worker;
	}

	QString newId = m_poolId + "worker_" + QString::number(m_workers.size() + 1);
	WebSocketWorker* newWorker = new WebSocketWorker(newId, m_wsUrl, this);
	
	QObject::connect(newWorker, &WebSocketWorker::messageReceived, this, &WebSocketPool::onWorkerMessage);
	QObject::connect(newWorker, &WebSocketWorker::statusChanged, this, &WebSocketPool::onWorkerStatus);
	m_workers.append(newWorker);
	qDebug() << "[WsPool] Created new worker:" << newId
		<< "| Total workers:" << m_workers.size();
	return newWorker;
}
