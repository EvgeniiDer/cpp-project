#include"ChartContainer.h"
#include"../../core/events/EventBus.h"
#include"FastChart.h"
#include<QLineEdit>
#include<QVBoxLayout>
#include<QHBoxLayout>
#include<QDebug>
#include"../components/SymbolLineEdit.h"
#include"../../core/managers/MarketDataManager.h"
#include"../components/TimeFrameWidget.h"
ChartContainer::ChartContainer(MarketDataManager* dataManager, const QString& exchange, const QString& symbol,const QString& marketType, QWidget* parent /* = nullptr */)
	: QWidget(parent)
	, m_dataManager(dataManager)
	, m_exchange(exchange)
	, m_marketType(marketType)
	, m_linkGroupId(0)
{
	static std::atomic<int> s_nextChartId{ 1 };
	m_chartId = s_nextChartId.fetch_add(1);
	setupUi();
	
	if (m_chart)
	{
		m_chart->setChartId(m_chartId);
		m_chart->setContext(m_dataManager, m_exchange, symbol, m_marketType);
	}
	if (m_symbolInput)
	{
		m_symbolInput->setText(symbol);
	}
	if (m_timeFrameWidget && m_chart)
	{
		QObject::connect(m_timeFrameWidget, &TimeFrameWidget::intervalChanged, m_chart, &FastChart::switchInterval);
	}
	QObject::connect(&EventBus::instance(), &EventBus::symbolChanged, this, [this](const QString& exchange, const QString& symbol, int groupId)
		{
			if (m_linkGroupId > 0 && groupId == m_linkGroupId && exchange == m_exchange)
			{
				qDebug() << "[ChartContainer] Group match found for group: " << m_linkGroupId << "Syncing to:" << symbol;
				m_chart->switchSymbol(m_exchange, symbol, m_marketType);
				m_symbolInput->setText(symbol);
			}
		});
}
void ChartContainer::setupUi()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(2);

	QHBoxLayout* topPanelLayout = new QHBoxLayout();
	topPanelLayout->setContentsMargins(4, 4, 4, 2);

	m_symbolInput = new SymbolLineEdit(m_exchange, m_marketType, this);
	
	if (m_dataManager)
	{
		QList<std::pair<QString,QString>> cached = m_dataManager->getCachedSymbols(m_exchange);
		if (!cached.isEmpty())
		{
			m_symbolInput->setSymbolList(cached);
		}
	}
	QObject::connect(m_symbolInput, &QLineEdit::returnPressed, this, &ChartContainer::onSymbolInputEntered);

	topPanelLayout->addWidget(m_symbolInput);
	m_timeFrameWidget = new TimeFrameWidget(this);
	topPanelLayout->addWidget(m_timeFrameWidget);
	topPanelLayout->addStretch();
	m_chart = new FastChart(this);

	mainLayout->addLayout(topPanelLayout);
	mainLayout->addWidget(m_chart, 1);
}
void ChartContainer::onSymbolInputEntered()
{
	QString newSymbol = m_symbolInput->text().toUpper().trimmed();
	if (newSymbol.isEmpty()) return;
	qDebug() << "[ChartContainer] Local switch to: " << newSymbol;
	m_chart->switchSymbol(m_exchange, newSymbol, m_marketType);
	if (m_linkGroupId > 0)
	{
		qDebug() << "[ChartContainer] Broadcasting symbol to group:" << m_linkGroupId;
		emit EventBus::instance().symbolChanged(m_exchange, newSymbol, m_linkGroupId);
	}
}