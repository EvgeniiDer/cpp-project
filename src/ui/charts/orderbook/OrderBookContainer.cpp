#include "OrderBookContainer.h"
#include "OrderBookBase.h"
#include "OrderBookClassic.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

#include "../../../core/events/EventBus.h"
#include "../../../core/managers/MarketDataManager.h"
#include "../../../core/network/common/NetworkTypes.h"
#include "../../components/SymbolLineEdit.h"

OrderBookContainer::OrderBookContainer(MarketDataManager* dataManager,
                                       const MarketInstrument& instrument,
                                       QWidget* parent)
    : QWidget(parent)
    , m_dataManager(dataManager)
    , m_instrument(instrument)
{
    static std::atomic<int> s_nextId{ 1 };
    m_containerId = s_nextId.fetch_add(1);

    setupUi();

    QObject::connect(&EventBus::instance(), &EventBus::orderBookReceived,
        this, &OrderBookContainer::onOrderBookReceived,
        Qt::UniqueConnection);

    QObject::connect(&EventBus::instance(), &EventBus::symbolChanged,
        this, [this](const QString& exchange, const QString& sym, int groupId)
        {
            if (m_linkGroupId > 0 && groupId == m_linkGroupId
                                  && exchange == m_instrument.exchange)
            {
                MarketInstrument updated = m_instrument;
                updated.symbol = sym;
                switchSymbol(updated);
                m_symbolInput->setText(sym);
            }
        });

    subscribe(m_instrument.symbol);
}

OrderBookContainer::~OrderBookContainer()
{
    if (m_dataManager && !m_instrument.symbol.isEmpty())
        unsubscribe(m_instrument.symbol);
}

// ─────────────────────────────────────────────────────────────────────────────
void OrderBookContainer::setupUi()
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
        this, &OrderBookContainer::onSymbolInputEntered);

    topLayout->addWidget(m_symbolInput);
    topLayout->addStretch();

    m_orderBook = new OrderBookClassic(this);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_orderBook, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
void OrderBookContainer::onSymbolInputEntered()
{
    const QString newSymbol = m_symbolInput->text().toUpper().trimmed();
    if (newSymbol.isEmpty()) return;

    MarketInstrument updated = m_instrument;
    updated.symbol = newSymbol;
    switchSymbol(updated);

    if (m_linkGroupId > 0)
        emit EventBus::instance().symbolChanged(m_instrument.exchange, newSymbol, m_linkGroupId);
}

void OrderBookContainer::onOrderBookReceived(const QString& exchange, const QString& symbol,
                                              const OrderBookSnapshot& snapshot, bool isDelta)
{
    if (exchange != m_instrument.exchange || symbol != m_instrument.symbol) return;

    if (isDelta)
        m_orderBook->applyDelta(snapshot);
    else
        m_orderBook->setData(snapshot);
}

void OrderBookContainer::switchSymbol(const MarketInstrument& instrument)
{
    if (m_instrument == instrument) return;
    unsubscribe(m_instrument.symbol);
    m_instrument = instrument;
    m_orderBook->setData(OrderBookSnapshot{});
    subscribe(m_instrument.symbol);
}

void OrderBookContainer::subscribe(const QString& symbol)
{
    if (!m_dataManager || symbol.isEmpty()) return;
    MarketContext ctx;
    ctx.chartId    = m_containerId;
    ctx.exchange   = m_instrument.exchange;
    ctx.symbol     = symbol;
    ctx.marketType = m_instrument.marketType;
    ctx.streamType = StreamType::OrderBook;
    m_dataManager->subscribeToStream(ctx);
}

void OrderBookContainer::unsubscribe(const QString& symbol)
{
    if (!m_dataManager || symbol.isEmpty()) return;
    MarketContext ctx;
    ctx.chartId    = m_containerId;
    ctx.exchange   = m_instrument.exchange;
    ctx.symbol     = symbol;
    ctx.marketType = m_instrument.marketType;
    ctx.streamType = StreamType::OrderBook;
    m_dataManager->unsubscribeFromStream(ctx);
}

void OrderBookContainer::setLinkGroupId(int groupId)
{
    m_linkGroupId = groupId;
}
