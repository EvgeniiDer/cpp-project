#pragma once
#include<QLineEdit>
#include<QList>
#include"../../core/events/EventBus.h"
class QCompleter;

class SymbolLineEdit : public QLineEdit
{
	Q_OBJECT
public:
	explicit SymbolLineEdit(const QString& exchange, const QString& marketType, QWidget* parent = nullptr);
	~SymbolLineEdit() = default;
	void setSymbolList(const QList<std::pair<QString, QString>>& symbols);
private slots:
	void onAvailableSymbolsLoaded(const QString& exchange,const QList<std::pair<QString,QString>>& symbols);
private:
	QString m_exchange;
	QString m_marketType;
	QCompleter* m_complete = nullptr;
};