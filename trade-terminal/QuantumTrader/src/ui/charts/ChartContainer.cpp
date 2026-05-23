#include"ChartContainer.h"
#include"../../core/events/EventBus.h"
#include"FastChart.h"
#include<QLineEdit>
#include<QVBoxLayout>
#include<QHBoxLayout>
#include<QDebug>

ChartContainer::ChartContainer(MarketDataManager* dataManager, const QString& exchange, const QString& symbol, QWidget* parent /* = nullptr */)
	: QWidget(parent)
	, m_dataManager(dataManager)
	, m_exchange(exchange)
	, m_linkGroupId(0)
{
	setupUi();
	
	if (m_chart)
	{
		m_chart->setContext(m_dataManager, m_exchange, symbol);
	}
	if (m_symbolInput)
	{
		m_symbolInput->setText(symbol);
	}
}
void ChartContainer::setupUi()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(2);

	QHBoxLayout* topPanelLayout = new QHBoxLayout();
	topPanelLayout->setContentsMargins(4, 4, 4, 2);

	m_symbolInput = new QLineEdit(this);
	m_symbolInput->setPlaceholderText("Enter symbol...");
	m_symbolInput->setFixedWidth(120);

	m_symbolInput->setStyleSheet("QLineEdit { font-weight: bold; text-transform: uppercase; }"); 

	QObject::connect(m_symbolInput, &QLineEdit::returnPressed, this, &ChartContainer::onSymbolInputEntered);

	topPanelLayout->addWidget(m_symbolInput);
	topPanelLayout->addStretch();
	m_chart = new FastChart(this);

	mainLayout->addLayout(topPanelLayout);
	mainLayout->addWidget(m_chart, 1);
}
void ChartContainer::onSymbolInputEntered()
{
	QString newSymbol = m_symbolInput->text().toUpper().trimmed();
	if (newSymbol.isEmpty()) return;
	qDebug() << "[ChartContainer] Input submitted. Broadcasting new symbol: " << newSymbol << " to group: " << m_linkGroupId;
	emit EventBus::instance().symbolChanged(m_exchange, newSymbol, m_linkGroupId);
}