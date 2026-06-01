#include"QssEditor.h"
#include<QKeyEvent>
#include<QAbstractItemView>
#include<QtGlobal>
#include<QModelIndex>
#include<QScrollbar>
#include<QCompleter>
#include<QDebug>




QssEditor::QssEditor(QWidget* parent) : QPlainTextEdit(parent)
{
	QFont font("Consolas", 12);
	font.setStyleHint(QFont::Monospace);
	this->setFont(font);
}


void QssEditor::setCompleter(QCompleter* completer)
{
	if (m_completer)
	{
		m_completer->disconnect(this);
	}

	m_completer = completer;
	if (!m_completer)
	{
		return;
	}

	m_completer->setWidget(this);
	m_completer->setCompletionMode(QCompleter::PopupCompletion);
	m_completer->setCaseSensitivity(Qt::CaseInsensitive);

	void (QCompleter:: * signalActivated)(const QString&) = &QCompleter::activated;

	connect(m_completer, signalActivated, this, [this](const QString& text)
		{
			this->insertCompletion(text);
		});
	
}
QCompleter* QssEditor::getCompleter()const
{
	return m_completer;
}

void QssEditor::insertCompletion(const QString& completion)
{
	if (m_completer->widget() != this)
	{
		return;
	}
	QTextCursor txtCursor = textCursor();
	int extra = completion.length() - m_completer->completionPrefix().length();
	txtCursor.movePosition(QTextCursor::Left);
	txtCursor.movePosition(QTextCursor::EndOfWord);
	txtCursor.insertText(completion.right(extra));
	setTextCursor(txtCursor);
	/*Как это работает по шагам (Магия синхронизации):
	В keyPressEvent: Когда ты нажимаешь клавишу, ты сам вызываешь m_completer->setCompletionPrefix(completionPrefix);. Ты буквально "записываешь" в память комплетера: «Сейчас префикс равен "ba"».
В момент выбора: Ты кликаешь на слово background.
Выстреливает Сигнал: activated("background").
Срабатывает твой Слот (Лямбда):
Он берет "background" (аргумент).
Он лезет в m_completer и дергает метод completionPrefix().
Комплетер отвечает: «У меня записано, что префикс "ba"».
Слот считает: 10 (background) - 2 (ba) = 8.
Слот отрезает 8 символов справа (ckground) и вставляет их.*/
}
QString QssEditor::textUnderCursor()const
{
	QTextCursor txtCursor = textCursor();
	txtCursor.select(QTextCursor::WordUnderCursor);
	return txtCursor.selectedText();
}
void QssEditor::focusInEvent(QFocusEvent* event)
{
	if (m_completer)
	{
		m_completer->setWidget(this);
	}
	QPlainTextEdit::focusInEvent(event);
}
void QssEditor::keyPressEvent(QKeyEvent* event)
{
	if (m_completer && m_completer->popup()->isVisible())
	{
		qDebug() << QObject::tr("[QssEditor] QCompleter Filter does not work");
		switch (event->key())
		{
		case Qt::Key_Enter:
		case Qt::Key_Return:
		case Qt::Key_Escape:
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
			event->ignore();
			return;
		default:
			break;
		}
	}
	QPlainTextEdit::keyPressEvent(event);

	bool isShortcut = false;
	isShortcut = ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_E);
	if (!m_completer || (isShortcut && !m_completer->popup()->isVisible()))
	{
		return;
	}
	static const QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-=");
	
	bool hasModifier = false;
	hasModifier = (event->modifiers() != Qt::NoModifier) && !isShortcut;
	QString completionPrefix = textUnderCursor();

	if (!isShortcut && (hasModifier || event->text().isEmpty() || completionPrefix.length() < 2
		|| eow.contains(event->text().right(1))))
	{
		m_completer->popup()->hide();
		return;
	}
	if (completionPrefix != m_completer->completionPrefix())
	{
		m_completer->setCompletionPrefix(completionPrefix);
		m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
	}

	QRect crRect = cursorRect();
	crRect.setWidth(m_completer->popup()->sizeHintForColumn(0)
		+ m_completer->popup()->verticalScrollBar()->sizeHint().width());
	m_completer->complete(crRect);

}
