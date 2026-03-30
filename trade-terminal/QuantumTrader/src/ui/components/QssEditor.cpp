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

QssEditor::~QssEditor(){}

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

	bool isShortcut = ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_E);
	if (!m_completer || (isShortcut && !m_completer->popup()->isVisible()))
	{
		return;
	}
	static const QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-=");

	bool hasModifier = (event->modifiers() != Qt::NoModifier) && !isShortcut;

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
