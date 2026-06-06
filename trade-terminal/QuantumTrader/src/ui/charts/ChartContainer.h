#pragma once
#include<QWidget>

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
	explicit ChartContainer(MarketDataManager* dataManager, const QString& exchange, const QString& symbol, const QString& marketType = "PERP", QWidget * parent = nullptr);
private:
	int m_chartId = 0;
	MarketDataManager* m_dataManager = nullptr;
	
	QString m_exchange;
	QString m_marketType;
	int m_linkGroupId;

	SymbolLineEdit* m_symbolInput = nullptr;
	TimeFrameWidget* m_timeFrameWidget = nullptr;
	FastChart* m_chart = nullptr;
		
	void setupUi();
private slots:
	void onSymbolInputEntered();
};