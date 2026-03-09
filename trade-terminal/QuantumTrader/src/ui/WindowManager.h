#pragma once
#include<QObject>
#include<QMdiArea>
#include<QMainWindow>
#include<QHash>
#include<QString>
#include<functional>

using WidgetFactory = std::function<QWidget* (QWidget*)>;

class WindowManager : public QObject
{
	Q_OBJECT
public:
	WindowManager(QMainWindow* mainWindow);
	void registryFactory(const QString& windowType, WidgetFactory factory);
	void createWindow(const QString& windowType);
private:
	QMainWindow* m_mainWindow;
	QHash<QString, WidgetFactory> m_factories;
	int m_windowCounter;
};
