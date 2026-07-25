#include "TimeAndSalesContainer.h"
#include "TimeAndSalesWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

#include "core/events/EventBus.h"
#include "core/managers/MarketDataManager.h"
#include "ui/components/SymbolLineEdit.h"

TimeAndSalesContainer::TimeAndSalesContainer(MarketDataManager* dataManager,
                                             const MarketInstrument& instrument,
                                             QWidget* parent)
    : QWidget(parent)
    , m_dataManager(dataManager)
    , m_instrument(instrument)
{
    static std::atomic<int> s_nextId{ 1 };
    m_containerId = s_nextId.fetch_add(1);

    setupUi();

    QObject::connect(&EventBus::instance(), &EventBus::tradeReceived,
        this, &TimeAndSalesContainer::onTradeReceived,
        Qt::UniqueConnection);

    QObject::connect(&EventBus::instance(), &EventBus::symbolChanged,
        this, [this](const QString& exchange, const QString& sym, int /*groupId*/)
        {
            if (exchange == m_instrument.exchange)
            {
                MarketInstrument updated = m_instrument;
                updated.symbol = sym;
                switchSymbol(updated);
                m_symbolInput->setText(sym);
            }
        });

    subscribe(m_instrument.symbol);
}

TimeAndSalesContainer::~TimeAndSalesContainer()
{
    if (m_dataManager && !m_instrument.symbol.isEmpty())
        unsubscribe(m_instrument.symbol);
}

void TimeAndSalesContainer::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);

    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(4, 4, 4, 2);

    m_symbolInput = new SymbolLineEdit(m_instrument, this);
    m_symbolInput->setText(m_instrument.symbol);

    if (m_dataManager)
    {
        const auto cached = m_dataManager->getCachedSymbols(m_instrument.exchange);
        if (!cached.isEmpty())
            m_symbolInput->setSymbolList(cached);
    }

    QObject::connect(m_symbolInput, &QLineEdit::returnPressed,
        this, &TimeAndSalesContainer::onSymbolInputEntered);

    topLayout->addWidget(m_symbolInput);
    topLayout->addStretch();

    m_widget = new TimeAndSalesWidget(this);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_widget, 1);
}

void TimeAndSalesContainer::onSymbolInputEntered()
{
    const QString newSym = m_symbolInput->text().toUpper().trimmed();
    if (newSym.isEmpty()) return;
    MarketInstrument updated = m_instrument;
    updated.symbol = newSym;
    switchSymbol(updated);
    emit EventBus::instance().symbolChanged(m_instrument.exchange, newSym, 0);
}

void TimeAndSalesContainer::onTradeReceived(const QString& exchange,
                                             const QString& symbol,
                                             const TradeTick& tick)
{
    if (exchange != m_instrument.exchange || symbol != m_instrument.symbol) return;
    m_widget->addTick(tick);
}

void TimeAndSalesContainer::switchSymbol(const MarketInstrument& instrument)
{
    if (m_instrument == instrument) return;
    unsubscribe(m_instrument.symbol);
    m_instrument = instrument;
    m_widget->clear();
    subscribe(m_instrument.symbol);
}

void TimeAndSalesContainer::subscribe(const QString& symbol)
{
    if (!m_dataManager || symbol.isEmpty()) return;
    MarketContext ctx;
    ctx.chartId    = m_containerId;
    ctx.exchange   = m_instrument.exchange;
    ctx.symbol     = symbol;
    ctx.marketType = m_instrument.marketType;
    ctx.streamType = StreamType::Trades;
    m_dataManager->subscribeToStream(ctx);
}

void TimeAndSalesContainer::unsubscribe(const QString& symbol)
{
    if (!m_dataManager || symbol.isEmpty()) return;
    MarketContext ctx;
    ctx.chartId    = m_containerId;
    ctx.exchange   = m_instrument.exchange;
    ctx.symbol     = symbol;
    ctx.marketType = m_instrument.marketType;
    ctx.streamType = StreamType::Trades;
    m_dataManager->unsubscribeFromStream(ctx);
}
