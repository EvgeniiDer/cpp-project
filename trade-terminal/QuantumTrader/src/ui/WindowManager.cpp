#include"WindowManager.h"
#include<QDebug>
#include<QDockWidget>

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

    QWidget* widget = m_factories[windowType](m_mainWindow);
    widget->setMinimumSize(600, 400);
    // 2. Оборачиваем его в магнитную док-панель
    QDockWidget* dock = new QDockWidget(windowType + " " + QString::number(m_windowCounter++), m_mainWindow);
    dock->setWidget(widget);

    // 3. Разрешаем отрывать на другой монитор и прилипать к любым краям
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);

    // 4. Очистка памяти при закрытии на крестик
    dock->setAttribute(Qt::WA_DeleteOnClose);

    // 5. Добавляем график в окно (сначала кидаем все новые графики вправо)
    m_mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dock);
    dock->show();
}
