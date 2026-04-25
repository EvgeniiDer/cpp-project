#pragma once
#include<QObject>
#include<QMainWindow>
#include<QHash>
#include<QString>
#include<functional>

namespace ads
{
	class CDockManager;
	class CDockWidget;
}
class MarketDataManager;

using WidgetFactory = std::function<QWidget* (QWidget*)>;

class WindowManager : public QObject
{
	Q_OBJECT
public:
	WindowManager(QMainWindow* mainWindow,MarketDataManager* dataManager, ads::CDockManager* dockManager = nullptr);
	void registryFactory(const QString& windowType, WidgetFactory factory);
	void createWindow(const QString& windowType);
private:
	QMainWindow* m_mainWindow{ nullptr };
	ads::CDockManager* m_dockManager{ nullptr };
	QHash<QString, WidgetFactory> m_factories;
	int m_windowCounter;

	MarketDataManager* m_dataManager{ nullptr };
};
