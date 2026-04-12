#pragma once

#include<QObject>
#include<QMap>

class QString;

enum class LinkGroup
{
	None = 0,
	Red,
	Blue,
	Green,
	Yellow
};

class LinkManager : public QObject
{
	Q_OBJECT
public:
	explicit LinkManager(QObject* pattern = nullptr);
	QString currentSymbolForGroup(LinkGroup group)const;

public slots:
	void changeGroupSymbol(LinkGroup, const QString& symbol, QObject* sender);
signals:
	void symbolBroadcasted(LinkGroup, const QString& symbol, QObject* sender);
private:
	QMap<LinkGroup, QString> m_groupSymbol;
};