#pragma once

#include<QMainWindow>
namespace ads
{
	class CDockManager;
}
class WindowManager;
class ActionManager;
class BybitConnector;
class MarketDataManager;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;
private:
	void setupUi();
	void setupOpenGLWarmup();
	void setupUIManagers();

	void setupTheme();
	void createMenus();

	ads::CDockManager* m_dockManager{ nullptr };
	WindowManager* m_windowManager{ nullptr };
	ActionManager* m_actionManager{ nullptr };
	BybitConnector* m_connector{ nullptr };
	MarketDataManager* m_dataManager{ nullptr };

	void setupNetworking();
	void setupManagers();
	void setupConnections();
};
