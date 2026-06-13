#include"SymbolLineEdit.h"
#include<QCompleter>

SymbolLineEdit::SymbolLineEdit(const MarketInstrument& instrument, QWidget* parent /* = nullptr */)
	: QLineEdit(parent)
	, m_instrument(instrument)
{
	this->setPlaceholderText("Symbol...");
	this->setFixedWidth(120);
	this->setStyleSheet("QLineEdit { font-weight: bold; text-transform: uppercase; }");

	QObject::connect(&EventBus::instance(), &EventBus::availableSymbolsLoaded, this, &SymbolLineEdit::onAvailableSymbolsLoaded);
}

void SymbolLineEdit::setSymbolList(const QList<std::pair<QString, QString>>& symbols)
{
	QStringList filteredList;
	const QString exch = m_instrument.exchange.toUpper();

	for (const std::pair<QString, QString>& pair : symbols)
	{
		const QString& symbolName = pair.first;
		const QString& symbolMarket = pair.second;
		if (exch.toUpper() == "BYBIT")
		{
			if (m_instrument.marketType == "PERP")
			{
				if (symbolMarket == "PERP" || symbolMarket == "INV_PERP")
				{
					filteredList.append(symbolName);
				} else
				{
					if (symbolMarket == m_instrument.marketType)
					{
						filteredList.append(symbolName);
					}
				}
			}
		} else if(exch.toUpper() == "FINAM" || exch.toUpper() == "ALOR")
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
	if (exchange.toUpper() != m_instrument.exchange.toUpper())
	{
		return;
	}
	this->setSymbolList(symbols);
}