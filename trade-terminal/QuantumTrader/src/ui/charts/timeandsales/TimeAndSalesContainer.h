#pragma once
#include <QWidget>
#include <QPointer>
#include <atomic>
#include "TimeAndSalesDataTypes.h"
#include "core/network/common/NetworkTypes.h"

class MarketDataManager;
class SymbolLineEdit;
class TimeAndSalesWidget;

class TimeAndSalesContainer : public QWidget
{
    Q_OBJECT
public:
    explicit TimeAndSalesContainer(MarketDataManager* dataManager,
                                   const MarketInstrument& instrument,
                                   QWidget* parent = nullptr);
    ~TimeAndSalesContainer() override;

public slots:
    void onTradeReceived(const QString& exchange, const QString& symbol,
                         const TradeTick& tick);
private slots:
    void onSymbolInputEntered();

private:
    void setupUi();
    void subscribe(const QString& symbol);
    void unsubscribe(const QString& symbol);
    void switchSymbol(const MarketInstrument& instrument);

    QPointer<MarketDataManager> m_dataManager;
    TimeAndSalesWidget* m_widget      = nullptr;
    SymbolLineEdit*     m_symbolInput = nullptr;

    MarketInstrument m_instrument;
    int m_containerId = 0;
};
