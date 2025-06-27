#include <QCoreApplication>
#include <iostream>
#include"pricemonitor.h"
int main(int argc, char *argv[])
{
    std::cout << "Hellow" << std::endl;
    QCoreApplication a(argc, argv);
    // --- Параметры для мониторинга ---
    QString giftIdToMonitor = "54364679-479b-49d7-a12a-61e7ad2492d5";
    QString authToken = "465d4057-2abb-485f-9882-a09a428559cd"; // ВАЖНО: этот токен может устареть!
    int checkIntervalMs = 5000; // Проверять каждые 5 секунд

    // --- Создаем и настраиваем наблюдателя ---
    PriceMonitor *monitor = new PriceMonitor(giftIdToMonitor, authToken, &a);

    // --- Подключаем слоты для вывода информации в консоль ---
    QObject::connect(monitor, &PriceMonitor::priceUpdate, [](const QString& giftId, double price){
        qDebug() << "ОБНОВЛЕНИЕ ЦЕНЫ для" << giftId << "-> Новая цена:" << QString::number(price, 'f', 4) << "TON";
    });

    QObject::connect(monitor, &PriceMonitor::saleStatusChanged, [](const QString& giftId, bool isOnSale){
        qDebug() << "СТАТУС ПРОДАЖИ для" << giftId << "-> В продаже:" << (isOnSale ? "ДА" : "НЕТ");
    });

    QObject::connect(monitor, &PriceMonitor::requestError, [](const QString& giftId, const QString& errorString){
        qWarning() << "ОШИБКА для" << giftId << "->" << errorString;
    });

    // --- Запускаем процесс ---
    monitor->startMonitoring(checkIntervalMs);

    return a.exec();
}
