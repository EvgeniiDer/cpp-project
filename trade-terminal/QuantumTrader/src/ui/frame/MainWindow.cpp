#include"MainWindow.h"
#include"../charts/FastChart.h"
#include"WindowManager.h"
#include"../../utils/ThemeManager.h"
#include"../components/ThemeEditorWidget.h"
#include<QApplication>
#include<QMenuBar>
#include<QMenu>
#include<QAction>
#include"../../core/models/Candle.h"
#include"../../utils/MockDataGenerator.h"
#include <DockManager.h>
#include <DockWidget.h>
MainWindow::MainWindow(QWidget* parent): QMainWindow(parent)
{

	setupUi();
	setupOpenGLWarmup(); //скрытый виджет что бы не забыть для того что бы все сразу началось обрабатываться видеокартой а не центральным процессором что бы не было мерцаиня
	setupManagers();
	setupTheme();

	createMenus();
	//createDockWindows();
}
MainWindow::~MainWindow()
{

}

void MainWindow::setupUi()
{
	resize(1200, 800);
	setWindowTitle("QuantumTrader Pro");
}
void MainWindow::setupOpenGLWarmup()
{

	QOpenGLWidget* warmUpwidget = new QOpenGLWidget(this);
	warmUpwidget->setFixedSize(1, 1);
	warmUpwidget->hide();
}
void MainWindow::setupTheme()
{
	connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](const QString& style)
		{
		//	m_dockManager->setStyleSheet(style);
			qApp->setStyleSheet(style);

			// В будущем сюда же добавим:
			// m_watchlist->setStyleSheet(style);
			// m_orderBook->setStyleSheet(style);
		});
	ThemeManager::instance().setTheme(ThemeManager::Theme::Dark);
}
void MainWindow::setupManagers()
{

	m_dockManager = new ads::CDockManager(this);
	
	m_windowManager = new WindowManager(this, m_dockManager);
	m_windowManager->registryFactory("Chart", [](QWidget* parent) -> QWidget*
		{
			FastChart* chart = new FastChart(parent);
			std::vector<Candle>baseM1 = MockDataGenerator::generate1MinCandles(100);
			int currentTimeframe = 5;
			std::vector<Candle>m5Candles = MockDataGenerator::aggregateCandles(baseM1, 5);
			chart->loadData(m5Candles);
			return chart;
		});
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


	ads::CDockWidget* themeDock = new ads::CDockWidget(QObject::tr("Theme Editor"));
	themeDock->setWidget(new ThemeEditorWidget(themeDock));
	m_dockManager->addDockWidget(ads::RightDockWidgetArea, themeDock);
}
