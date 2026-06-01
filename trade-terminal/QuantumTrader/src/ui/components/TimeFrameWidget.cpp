#include"TimeFrameWidget.h"
#include<QHBoxLayout>
#include<QPushButton>
#include<QLineEdit>
#include<QDebug>
#include <windows.h>

// В Будущем удаляем все магические цифры !!!!
namespace
{
	constexpr int WidthStaticButton = 40;
	constexpr int WidthCustomButton = 45;
	constexpr int WidthMainPlusButton = 25;
	constexpr int WidthSubPlusButton = 20;
	constexpr int WidthCustomInput = 45;
	constexpr int LayoutSpacing = 6;
}

ChartInterval TimeFrameWidget::parseInterval(const QString& text)
{
	if (text.isEmpty()) return {ChartInterval::Unit::Minute, 1};

	int digitCount = 0;
	while (digitCount < text.length() && text[digitCount].isDigit())
	{	
		digitCount++;
	}
	int count = text.left(digitCount).toInt();
	QString suffix = text.mid(digitCount).trimmed();

	if (count <= 0) count = 1;

	static const QHash<QString, ChartInterval::Unit> unitRegistry =
		{
		{"s", ChartInterval::Unit::Second},
		{"m", ChartInterval::Unit::Minute},
		{"h", ChartInterval::Unit::Hour},
		{"d", ChartInterval::Unit::Day},
		{"w", ChartInterval::Unit::Week},
		{"M", ChartInterval::Unit::Month},
		{"t", ChartInterval::Unit::Tick},
		{"v", ChartInterval::Unit::Volume},
		{"r", ChartInterval::Unit::Range}
	};
	ChartInterval::Unit unit = unitRegistry.value(suffix, ChartInterval::Unit::Minute);
	return {unit, count};// Аналог ChartInterval(unit, count) проблема в том что компилятор уже знает сигнатуру мы ее уже определили это рекомендация от одного долбаеба который меня уже з.л продавил с.ка
}

TimeFrameWidget::TimeFrameWidget(QWidget* parent)
	: QWidget(parent)
{
	m_layout = new QHBoxLayout(this);
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->setSpacing(LayoutSpacing);

	QStringList staticTimeFrame = {"1m", "5m", "15m", "30m", "1h", "12h", "1d", "1w", "1M"};

	for (const QString& tf : staticTimeFrame)
	{
		QPushButton* tfBtn = new QPushButton(tf, this);
		tfBtn->setFixedWidth(WidthStaticButton);
		tfBtn->setCheckable(true);

		if (tf == "5m") tfBtn->setChecked(true);

		QObject::connect(tfBtn, &QPushButton::clicked, this, &TimeFrameWidget::onTimeframeClicked);
		m_layout->addWidget(tfBtn);
		m_tfButtons.append(tfBtn);
	}
	QPushButton* addTfBtn = new QPushButton("+", this);
	addTfBtn->setFixedWidth(WidthMainPlusButton);
	addTfBtn->setStyleSheet("QPushButton { font-weight: bold; color: #00ff00; }");
	addTfBtn->setToolTip("Add custom timeframe");
	
	QObject::connect(addTfBtn, &QPushButton::clicked, this, &TimeFrameWidget::onAddCustomTimeframeClicked);
	m_layout->addWidget(addTfBtn);
}

void TimeFrameWidget::onTimeframeClicked()
{
	QPushButton* clickedBtn = qobject_cast<QPushButton*>(sender());
	if (!clickedBtn) return;

	for (QPushButton* btn : m_tfButtons)
	{
		if (btn != clickedBtn)
		{
			btn->setChecked(false);
		}
	}
	ChartInterval interval = TimeFrameWidget::parseInterval(clickedBtn->text());
	emit intervalChanged(interval);
}

void TimeFrameWidget::onAddCustomTimeframeClicked()
{
	QPushButton* clickedBtn = qobject_cast<QPushButton*>(sender());
	if (!clickedBtn || !m_layout)return;

	clickedBtn->setEnabled(false);
	QLineEdit* customInput = new QLineEdit(this);
	customInput->setPlaceholderText("7m");
	customInput->setFixedWidth(WidthCustomInput);
	customInput->setStyleSheet("QLineEdit { font-weight: bold; color: #00ff00; text-align: center; }");

	QPushButton* subPlusBtn = new QPushButton("+", this);
	subPlusBtn->setFixedWidth(WidthSubPlusButton);
	subPlusBtn->setStyleSheet("QPushButton { font-weight: bold; color: #00ffff; }");

	int plusIndex = m_layout->indexOf(clickedBtn);
	m_layout->insertWidget(plusIndex, customInput);
	m_layout->insertWidget(plusIndex + 1, subPlusBtn);

	auto confirmLogic = [this, customInput, subPlusBtn, clickedBtn, plusIndex]()
		{
			QString text = customInput->text().trimmed();
			if (!text.isEmpty() && text[0].isDigit())
			{
				ChartInterval interval = TimeFrameWidget::parseInterval(text);
				emit intervalChanged(interval);

				QPushButton* newTfBtn = new QPushButton(text, this);
				newTfBtn->setFixedWidth(WidthCustomInput);
				newTfBtn->setCheckable(true);
				newTfBtn->setChecked(true);
				newTfBtn->setStyleSheet("QPushButton { border: 1px solid #00ff00; font-weight: bold; }");
				for (QPushButton* btn : m_tfButtons)
				{
					btn->setChecked(false);
				}
				QObject::connect(newTfBtn, &QPushButton::clicked, this, &TimeFrameWidget::onTimeframeClicked);
				m_layout->insertWidget(plusIndex, newTfBtn);
				m_tfButtons.append(newTfBtn);
			}
			customInput->deleteLater();
			subPlusBtn->deleteLater();
			clickedBtn->setEnabled(true);
		};
	QObject::connect(subPlusBtn, &QPushButton::clicked, confirmLogic);
	QObject::connect(customInput, &QLineEdit::returnPressed, confirmLogic);

}





