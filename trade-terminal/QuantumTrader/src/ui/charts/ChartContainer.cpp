#include "ChartContainer.h"
#include "../../core/events/EventBus.h"
#include "FastChart.h"
#include "../components/SymbolLineEdit.h"
#include "../../core/managers/MarketDataManager.h"
#include "../components/TimeFrameWidget.h"
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

ChartContainer::ChartContainer(MarketDataManager* dataManager,
    const QString& exchange,
    const QString& symbol,
    const QString& marketType,
    QWidget* parent)
    : QWidget(parent)
    , m_dataManager(dataManager)
    , m_linkGroupId(0)
{
    m_instrument.exchange = exchange;
    m_instrument.symbol = symbol;
    m_instrument.marketType = marketType;

    static std::atomic<int> s_nextChartId{ 1 };
    m_chartId = s_nextChartId.fetch_add(1);

    setupUi();

    if (m_chart)
    {
        m_chart->setChartId(m_chartId);
        m_chart->setContext(m_dataManager,
            m_instrument.exchange,
            m_instrument.symbol,
            m_instrument.marketType);
    }

    if (m_symbolInput)
        m_symbolInput->setText(m_instrument.symbol);

    if (m_timeFrameWidget && m_chart)
    {
        QObject::connect(m_timeFrameWidget, &TimeFrameWidget::intervalChanged,
            m_chart, &FastChart::switchInterval);
    }

    QObject::connect(&EventBus::instance(), &EventBus::symbolChanged, this,
        [this](const QString& exchange, const QString& symbol, int groupId)
        {
            if (m_linkGroupId > 0
                && groupId == m_linkGroupId
                && exchange == m_instrument.exchange)
            {
                qDebug() << "[ChartContainer] Group sync → group:" << m_linkGroupId
                    << "symbol:" << symbol;
                m_instrument.symbol = symbol;
                m_chart->switchSymbol(m_instrument.exchange,
                    symbol,
                    m_instrument.marketType);
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

    m_symbolInput = new SymbolLineEdit(m_instrument, this);

    if (m_dataManager)
    {
        const auto cached = m_dataManager->getCachedSymbols(m_instrument.exchange);
        if (!cached.isEmpty())
            m_symbolInput->setSymbolList(cached);
    }

    QObject::connect(m_symbolInput, &QLineEdit::returnPressed,
        this, &ChartContainer::onSymbolInputEntered);

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
    const QString newSymbol = m_symbolInput->text().toUpper().trimmed();
    if (newSymbol.isEmpty()) return;

    qDebug() << "[ChartContainer] Local switch to:" << newSymbol;
    m_instrument.symbol = newSymbol;
    m_chart->switchSymbol(m_instrument.exchange,
        newSymbol,
        m_instrument.marketType);

    if (m_linkGroupId > 0)
    {
        qDebug() << "[ChartContainer] Broadcasting symbol to group:" << m_linkGroupId;
        emit EventBus::instance().symbolChanged(m_instrument.exchange,
            newSymbol,
            m_linkGroupId);
    }
}