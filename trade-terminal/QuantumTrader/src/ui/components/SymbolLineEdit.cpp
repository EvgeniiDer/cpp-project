#include"SymbolLineEdit.h"
#include<QCompleter>

SymbolLineEdit::SymbolLineEdit(const QString& exchange, const QString& marketType, QWidget* parent /* = nullptr */)
	: m_exchange(exchange)
	, m_marketType(marketType)
	, QLineEdit(parent)
{
	this->setPlaceholderText("Symbol...");
	this->setFixedWidth(120);
	this->setStyleSheet("QLineEdit { font-weight: bold; text-transform: uppercase; }");

	QObject::connect(&EventBus::instance(), &EventBus::availableSymbolsLoaded, this, &SymbolLineEdit::onAvailableSymbolsLoaded);
}

void SymbolLineEdit::setSymbolList(const QList<std::pair<QString, QString>>& symbols)
{
	QStringList filteredList;
	for (const auto& pair : symbols)
	{
		const QString& symbolName = pair.first;
		const QString& symbolMarket = pair.second;
		if (m_exchange.toUpper() == "BYBIT")
		{
			if (m_marketType == "PERP")
			{
				if (symbolMarket == "PERP" || symbolMarket == "INV_PERP")
				{
					filteredList.append(symbolName);
				} else
				{
					if (symbolMarket == m_marketType)
					{
						filteredList.append(symbolName);
					}
				}
			}
		} else if(m_exchange.toUpper() == "FINAM" || m_exchange.toUpper() == "ALOR")
		{
			/// После написание коннектора АЛОР и ФИНАМ
		}
	}
	if (filteredList.isEmpty())return;
	
	if (m_complete)
	{
		delete m_complete;
	}

	m_complete = new QCompleter(filteredList, this);
	m_complete->setCaseSensitivity(Qt::CaseInsensitive);
	m_complete->setCompletionMode(QCompleter::PopupCompletion);
	m_complete->setMaxVisibleItems(13); // вывести потмо в настройки!!!

	this->setCompleter(m_complete);

}

void SymbolLineEdit::onAvailableSymbolsLoaded(const QString& exchange,const QList<std::pair<QString,QString>>& symbols)
{
	if (exchange.toUpper() != m_exchange.toUpper())
	{
		return;
	}
	this->setSymbolList(symbols);
}