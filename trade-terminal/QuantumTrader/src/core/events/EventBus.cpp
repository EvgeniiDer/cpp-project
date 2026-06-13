#include"EventBus.h"


EventBus::EventBus(QObject* parent)
	: QObject(parent)
{
	qRegisterMetaType<QList<std::pair<QString, QString>>>("QList<std::pair<QString,QString>>");
	qRegisterMetaType<StreamType>("StreamType");
	qRegisterMetaType<MarketInstrument>("MarketInstrument");
}
