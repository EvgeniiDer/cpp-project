#pragma once

#include<QPlainTextEdit>

class QCompleter;

class QssEditor : QPlainTextEdit
{
	Q_OBJECT
public:
	explicit QssEditor(QWidget* parent = nullptr);
	~QssEditor();

	void setCompleter(QCompleter* completer);
	QCompleter* getCompleter()const;
protected:
	void keyPressEvent(QKeyEvent* e)override;
	void focusInEvent(QFocusEvent* e)override;

private slots:
	void insertCompletion(const QString& completion);
private:
	QString textUnderCursor()const;

	QCompleter* m_completer = nullptr;
		
};