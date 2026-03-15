#include"MainWindow.h"
#include"charts/FastChart.h"
#include"WindowManager.h"

#include<QMenuBar>
#include<QMenu>
#include<QAction>

#include <DockManager.h>
#include <DockWidget.h>
MainWindow::MainWindow(QWidget* parent): QMainWindow(parent)
{

	setupUi();
	QOpenGLWidget* warmUpwidget = new QOpenGLWidget(this);
	warmUpwidget->setFixedSize(1, 1);
	warmUpwidget->hide();
	m_dockManager = new ads::CDockManager(this);
	m_windowManager = new WindowManager(this, m_dockManager);

	m_windowManager->registryFactory("Chart", [](QWidget* parent) -> QWidget*
		{
			return new FastChart(parent);
		});
	createMenus();
	createDockWindows();
}
MainWindow::~MainWindow()
{

}

void MainWindow::setupUi()
{
	resize(1200, 800);
	setWindowTitle("QuantumTrader Pro");
}
void MainWindow::createMenus()
{
	QMenu* fileMenu = menuBar()->addMenu(QObject::tr("File"));
	QAction* newChartAct = new QAction(QObject::tr("New Chart"), this);
	newChartAct->setShortcut(QKeySequence("Ctrl+N"));

	connect(newChartAct, &QAction::triggered, this, [this]()
		{
			m_windowManager->createWindow("Chart");
		});
	fileMenu->addAction(newChartAct);
}

void MainWindow::createDockWindows()
{

	ads::CDockWidget* propertiesDock = new ads::CDockWidget(QObject::tr("Properties"));
	QWidget* emptyWidget = new QWidget();
	emptyWidget->setStyleSheet("background-color: #1e1e1e;");
	propertiesDock->setWidget(emptyWidget);

	m_dockManager->addDockWidget(ads::RightDockWidgetArea, propertiesDock);
}
