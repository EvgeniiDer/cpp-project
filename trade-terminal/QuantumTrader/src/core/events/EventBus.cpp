#include"EventBus.h"


EventBus::EventBus(QObject* parent /* = nullptr */)
	: QObject(parent)
{
	qRegisterMetaType<QList<std::pair<QString, QString>>>("QList<std::pair<QString,QString>>");
}
