#pragma once
#include<QWidget>

class QLineEdit;
class QVBoxLayout;
class QHBoxLayout;
class FastChart;
class MarketDataManager;

class ChartContainer : public QWidget
{
	Q_OBJECT
public:
	explicit ChartContainer(MarketDataManager* dataManager, const QString& exchange, const QString& symbol, const QString& marketType = "PERP", QWidget * parent = nullptr);
	virtual ~ChartContainer() = default;
private:
	MarketDataManager* m_dataManager;
	QString m_exchange;
	QString m_marketType;
	int m_linkGroupId;

	QLineEdit* m_symbolInput;
	FastChart* m_chart;
	
	void setupUi();
private slots:
	void onSymbolInputEntered();
};