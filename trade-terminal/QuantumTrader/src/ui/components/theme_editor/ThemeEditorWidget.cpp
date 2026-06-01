#include"ThemeEditorWidget.h"
#include"QssEditor.h"
#include"CssHighlighter.h"

#include"../../../utils/ThemeManager.h"

#include<QVBoxLayout>
#include<QPushButton>
#include<QCompleter>
#include<QStringList>


ThemeEditorWidget::ThemeEditorWidget(QWidget* parent) : QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_editor = new QssEditor(this);

    new CssHighlighter(m_editor->document());

	QStringList words;
	words << "ads--CDockWidgetTab" << "ads--CDockAreaTabBar" << "ads--CDockOverlay"
		  << "background-color" << "color" << "border" << "padding" << "margin"
		  << "font-size" << "font-weight" << "bold" << "transparent";
	words.sort();

	QCompleter* completer = new QCompleter(words, m_editor);
	m_editor->setCompleter(completer);
	m_editor->setPlaceholderText(QObject::tr("Start typing CSS selectors... (e.g., add--"));

	layout->addWidget(m_editor);

	QPushButton* applyBtn = new QPushButton(QObject::tr("Apply Global Style"), this);
	applyBtn->setStyleSheet("padding: 10px; background-color: #007ACC; color: white; font-weight: bold; font-size: 14px;");
	
	layout->addWidget(applyBtn);

	connect(applyBtn, &QPushButton::clicked, this, [this]()
		{
			ThemeManager::instance().setRawStyle(m_editor->toPlainText());
		});

}

