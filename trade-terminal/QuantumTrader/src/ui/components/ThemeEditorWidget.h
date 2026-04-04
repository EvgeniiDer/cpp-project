#pragma once

#include<QWidget>

class  QssEditor;

class ThemeEditorWidget : public QWidget
{
	Q_OBJECT
private:
	QssEditor* m_editor;
public:
	explicit ThemeEditorWidget(QWidget* parent = nullptr);
	~ThemeEditorWidget() {};
};