#pragma once
#include <QWidget>
#include <QPointer>
#include <atomic>
#include "OrderBookDataTypes.h"
#include "core/network/common/NetworkTypes.h"

class MarketDataManager;
class SymbolLineEdit;
class OrderBookBase;

class OrderBookContainer : public QWidget
{
    Q_OBJECT
public:
    explicit OrderBookContainer(MarketDataManager* dataManager,
                                const MarketInstrument& instrument,
                                QWidget* parent = nullptr);
    ~OrderBookContainer() override;

    void setLinkGroupId(int groupId);
    int getLinkGroupId() const { return m_linkGroupId; }
    int getContainerId() const { return m_containerId; }

public slots:
    void onOrderBookReceived(const QString& exchange, const QString& symbol,
                             const OrderBookSnapshot& snapshot, bool isDelta);
private slots:
    void onSymbolInputEntered();

private:
    void setupUi();
    void subscribe(const QString& symbol);
    void unsubscribe(const QString& symbol);
    void switchSymbol(const MarketInstrument& instrument);

    QPointer<MarketDataManager> m_dataManager;
    OrderBookBase*  m_orderBook   = nullptr;
    SymbolLineEdit* m_symbolInput = nullptr;

    MarketInstrument m_instrument;
    int m_linkGroupId = 0;
    int m_containerId = 0;
};
