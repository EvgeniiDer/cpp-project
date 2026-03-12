#include"WindowManager.h"
#include<QDebug>
#include<QDockWidget>
#include<QLayout>

WindowManager::WindowManager(QMainWindow* mainWindow) : QObject(mainWindow), m_mainWindow(mainWindow), m_windowCounter(1)
{
	Q_ASSERT(m_mainWindow != nullptr);
}

void WindowManager::registryFactory(const QString& windowType, WidgetFactory factory)
{
	m_factories.insert(windowType, factory);
}
void WindowManager::createWindow(const QString& windowType)
{
	if (!m_factories.contains(windowType))
	{
		qWarning() << "Attempt to create an unknown window type :" << windowType;
		return;
	}
    
    
    
    QDockWidget* dock = new QDockWidget(windowType + " " + QString::number(m_windowCounter++), m_mainWindow);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock->setAttribute(Qt::WA_DeleteOnClose);
    
    QWidget* container = new QWidget(dock);
    QVBoxLayout* layout = new QVBoxLayout(container);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget* chartWidget = m_factories[windowType](container);
    chartWidget->setMinimumSize(600, 400);
    layout->addWidget(chartWidget);

    // 2. Оборачиваем его в магнитную док-панель
    dock->setWidget(container);

    m_mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dock);
    dock->show();
}
