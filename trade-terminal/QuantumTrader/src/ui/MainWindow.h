#pragma once

#include<QMainWindow>

class WindowManager;

class QDockWidget;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;
private:
	void setupUi();
	void createMenus();
	void createDockWindows();

	
	QDockWidget* m_propertiesDock;
	WindowManager* m_windowManager;
};
