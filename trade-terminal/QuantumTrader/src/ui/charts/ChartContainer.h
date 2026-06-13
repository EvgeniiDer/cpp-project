#pragma once
#include <QWidget>
#include <atomic>
#include "core/network/common/NetworkTypes.h"

class QLineEdit;
class QVBoxLayout;
class QHBoxLayout;
class FastChart;
class MarketDataManager;
class SymbolLineEdit;
class TimeFrameWidget;

class ChartContainer : public QWidget
{
    Q_OBJECT
public:
    explicit ChartContainer(MarketDataManager* dataManager,
        const QString& exchange,
        const QString& symbol,
        const QString& marketType = "PERP",
        QWidget* parent = nullptr);

    void setLinkGroupId(int groupId) { m_linkGroupId = groupId; }

private:
    int m_chartId = 0;
    int m_linkGroupId = 0;
    MarketDataManager* m_dataManager = nullptr;
    MarketInstrument   m_instrument;

    SymbolLineEdit* m_symbolInput = nullptr;
    TimeFrameWidget* m_timeFrameWidget = nullptr;
    FastChart* m_chart = nullptr;

    void setupUi();

private slots:
    void onSymbolInputEntered();
};