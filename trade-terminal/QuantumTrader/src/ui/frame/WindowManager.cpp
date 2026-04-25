#include"WindowManager.h"
#include<QDebug>
#include<QLayout>
#include<DockManager.h>
#include<DockWidget.h>
#include"../src/core/managers/MarketDataManager.h"
WindowManager::WindowManager(QMainWindow* mainWindow,MarketDataManager* dataManager ,ads::CDockManager* dockManager) 
        :QObject(mainWindow), 
         m_mainWindow(mainWindow), 
         m_dataManager(dataManager),
         m_dockManager(dockManager),
         m_windowCounter(1)// ДЛЯ Счетчика Открывших графиков
{
	Q_ASSERT(m_mainWindow != nullptr);
    if (!m_dockManager)
    {
        qCritical() << QObject::tr("[WindowManager] CRITICAL: CDockManager is nullptr");
    }
}

void WindowManager::registryFactory(const QString& windowType, WidgetFactory factory)
{
    if (!factory)
    {
        qWarning() << QObject::tr("[WindowManager] Attempt to register a null factory for: ") << windowType;
        return;
    }
	m_factories.insert(windowType, factory);
    qDebug() << QObject::tr("[WindowManager] Factory registered for:") << windowType;
}
void WindowManager::createWindow(const QString& windowType)
{
    if (!m_dockManager)
    {
        qWarning() << QObject::tr("[WindowManager] Cannot create window, DockManager is missing!");
        return;
    }
	if (!m_factories.contains(windowType))
	{
		qWarning() << QObject::tr("[WindowManager] Attempt to create an unknown window type:") << windowType;
		return;
	}
    QString title = windowType + " " + QString::number(m_windowCounter++);
       
    ads::CDockWidget* dock = new ads::CDockWidget(title);
    dock->setAttribute(Qt::WA_DeleteOnClose);

    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget* chartWidget = m_factories[windowType](container);

    if (!chartWidget)
    {
        qCritical() << QObject::tr("[WindowManager] Factory failed to create widget for: ") << windowType;
        delete container;
        container = nullptr;
        delete dock;
        dock = nullptr;
        return;
    }
    chartWidget->setMinimumSize(600, 400);
    layout->addWidget(chartWidget);
    dock->setWidget(container);
    m_dockManager->addDockWidget(ads::CenterDockWidgetArea, dock);
    dock->show();
}
