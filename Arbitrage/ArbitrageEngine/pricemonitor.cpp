#include "pricemonitor.h"
#include<QNetworkRequest>
#include<QNetworkReply>
#include<QJsonDocument>
#include<QJsonObject>
#include<QUrl>
#include<QDebug>


PriceMonitor::PriceMonitor(const QString &giftId, const QString &authToken, QObject *parent)
    : QObject(parent), m_giftId(giftId), m_authToken(authToken)
{
    m_networkManager = new QNetworkAccessManager(this);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PriceMonitor::performRequest);
}

void PriceMonitor::startMonitoring(int intervalMs)
{
    qDebug() << "Start monitoring" << m_giftId << "interval: " << intervalMs << "mc";
    m_timer->start(intervalMs);
    performRequest();
}
void PriceMonitor::stopMonitoring()
{
    m_timer->stop();
    qDebug() << "Stop monitoring id: " << m_giftId;
}
void PriceMonitor::performRequest()
{

    //Формируем запрос(https:/...)
    QUrl url("....");
    QNetworkRequest request(url);

    //Устанавливаем заголовок авторизации
    request.setRawHeader("Authorization", m_authToken.toUtf8());

    //Отправляем Get Запрос
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &PriceMonitor::onReplyFinished);
}
void PriceMonitor::onReplyFinished()
{
    //Получаем ответ на указатель ответа
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if(!reply) return;

    if(reply->error() == QNetworkReply::NoError)
    {
        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        QJsonObject jsonObj = jsonDoc.object();

        //Извлекаем данные
        bool isOnSale = jsonObj.value("isOnSale").toBool();
        emit saleStatusChanged(m_giftId, isOnSale);

        if(isOnSale)
        {
            qint64 salePriceNano = jsonObj.value("salePrice").toVariant().toLongLong();
            double price = static_cast<double>(salePriceNano) / 1000000000.0;

            //Отправляем сигнал об имзенении цены
            emit priceUpdate(m_giftId, price);
        }
    }
    else{
        //Отправляем сигнал об Ошибки
        emit requestError(m_giftId, reply->errorString());
    }
    reply->deleteLater();
}
