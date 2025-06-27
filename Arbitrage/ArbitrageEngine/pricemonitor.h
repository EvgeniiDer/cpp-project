#ifndef PRICEMONITOR_H
#define PRICEMONITOR_H

#include<QTimer>
#include<QString>
#include<QObject>
#include<QNetworkAccessManager>

class PriceMonitor  : public QObject
{
    Q_OBJECT
public:
    explicit PriceMonitor(const QString& giftId, const QString& authToken, QObject* parent = nullptr);

    void startMonitoring(int intervalMs);
    void stopMonitoring();
signals:
    void priceUpdate(const QString& giftId, double price);
    void saleStatusChanged(const QString& giftId, bool isOnSale);
    void requestError(const QString& giftId, const QString& errorString);
public slots:
    void performRequest();
    void onReplyFinished();
private:
    QString m_giftId;
    QString m_authToken;
    QTimer* m_timer;
    QNetworkAccessManager* m_networkManager;
};



#endif // PRICEMONITOR_H
