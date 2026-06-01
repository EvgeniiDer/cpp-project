#pragma once
#include<QWidget>
#include "core/network/common/NetworkTypes.h"

class QHBoxLayout;
class QPushButton;

class TimeFrameWidget : public QWidget
{
	Q_OBJECT
public:
	explicit TimeFrameWidget(QWidget* parent = nullptr);
	static  ChartInterval parseInterval(const QString& text);
signals:
	void intervalChanged(const ChartInterval& interval);
private slots:
	void onTimeframeClicked();
	void onAddCustomTimeframeClicked();
private:
	QHBoxLayout* m_layout = nullptr;
	QList<QPushButton*> m_tfButtons;
};