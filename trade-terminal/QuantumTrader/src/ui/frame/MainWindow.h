#pragma once

#include<QMainWindow>
namespace ads
{
	class CDockManager;
}
class WindowManager;
class ActionManager;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;
private:
	void setupUi();
	void setupOpenGLWarmup();
	void setupManagers();
	void setupTheme();
	void createMenus();

	ads::CDockManager* m_dockManager{ nullptr };
	WindowManager* m_windowManager{ nullptr };
	ActionManager* m_actionManager{ nullptr };
};
