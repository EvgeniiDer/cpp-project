#include"LinkManager.h"

#include<QString>


LinkManager::LinkManager(QObject* pattern /* = nullptr */) : QObject(pattern)
{

}

QString LinkManager::currentSymbolForGroup(LinkGroup group) const
{
	return m_groupSymbol.value(group, "");
}

void LinkManager::changeGroupSymbol(LinkGroup group, const QString& symbol, QObject* sender)
{
	if (group == LinkGroup::None)
	{
		return;
	}
	if (m_groupSymbol.value(group) == symbol)
	{
		return;
	}
	m_groupSymbol[group] = symbol;
	emit symbolBroadcasted(group, symbol, sender);
}