#include"MainWindow.h"
#include"../charts/FastChart.h"
#include"../frame/WindowManager.h"
#include"../frame/ActionManager.h"
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
MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
{
	m_dockManager = new ads::CDockManager(this);
	this->setCentralWidget(m_dockManager);
	m_windowManager = new WindowManager(this, m_dockManager);
	m_actionManager = new ActionManager(m_windowManager, this);
	setupUi();
	setupOpenGLWarmup(); //скрытый виджет что бы не забыть для того что бы все сразу началось обрабатываться видеокартой а не центральным процессором что бы не было мерцаиня
	setupManagers();
	setupTheme();

	createMenus();
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
	m_windowManager->registryFactory("Chart", [](QWidget* parent) -> QWidget*
		{
			FastChart* chart = new FastChart(parent);
			std::vector<Candle>baseM1 = MockDataGenerator::generate1MinCandles(100);
			int currentTimeframe = 5;
			std::vector<Candle>m5Candles = MockDataGenerator::aggregateCandles(baseM1, 5);
			chart->loadData(m5Candles);
			return chart;
		});
	m_windowManager->registryFactory("Properties", [](QWidget* parent) ->QWidget*
		{
			return new QWidget(parent);
		});
	m_windowManager->registryFactory("ThemeEditor", [](QWidget* parent) ->QWidget*
		{
			return new ThemeEditorWidget(parent);
		});
}
void MainWindow::createMenus()
{
	QMenu* fileMenu = menuBar()->addMenu(QObject::tr("&File"));

	fileMenu->addAction(m_actionManager->getAction("File.NewChart"));
	fileMenu->addSeparator();

	QAction* exitAction = fileMenu->addAction(QObject::tr("Exit"));
	connect(exitAction, &QAction::triggered, this, &QWidget::close);

	QMenu* toolsMenu = menuBar()->addMenu(QObject::tr("&Tools"));
	toolsMenu->addAction(m_actionManager->getAction("Tools.Properties"));
	toolsMenu->addAction(m_actionManager->getAction("Tools.ThemeEditor"));
}

