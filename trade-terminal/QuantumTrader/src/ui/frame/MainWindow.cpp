#include"MainWindow.h"
#include"../src/core/managers/MarketDataManager.h"
#include"../charts/FastChart.h"
#include"../frame/WindowManager.h"
#include"../frame/ActionManager.h"
#include"../../utils/ThemeManager.h"
#include"../components/ThemeEditorWidget.h"
#include<QApplication>
#include<QMenuBar>
#include<QMenu>
#include<QAction>
//#include"../../core/models/Candle.h"
#include <DockManager.h>
#include <DockWidget.h>
#include"../src/core/network/bybit/BybitConnector.h"
#include"../../core/events/EventBus.h"
#include"../charts/ChartContainer.h"
MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
{

	setupUi();
	setupOpenGLWarmup(); //скрытый виджет что бы не забыть для того что бы все сразу началось обрабатываться видеокартой а не центральным процессором что бы не было мерцаиня
	setupNetworking();
	setupUIManagers();

	setupConnections();
	setupTheme();
	createMenus();

	m_dataManager->connectTo("Bybit");
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
void MainWindow::setupNetworking()
{
	m_dataManager = new MarketDataManager(this);
	m_dataManager->registerFactory("Bybit", [](QObject* parent)
		{
			return new BybitConnector(parent);
		});
}
void MainWindow::setupUIManagers()
{
	m_dockManager = new ads::CDockManager(this);
	this->setCentralWidget(m_dockManager);
	m_windowManager = new WindowManager(this, m_dataManager, m_dockManager);
	m_actionManager = new ActionManager(m_windowManager, this);
	setupManagers();
}
void MainWindow::setupManagers()
{
	m_windowManager->registryFactory("Chart", [this](QWidget* parent) -> QWidget*
		{
			return new ChartContainer(this->m_dataManager, "Bybit", "BTCUSDT", parent);
		});
	m_windowManager->registryFactory("Properties", [](QWidget* parent) -> QWidget*
		{
			return new QWidget(parent); // Replace with your actual PropertiesWidget later
		});

	// Register Theme Editor (Don't forget this!)
	m_windowManager->registryFactory("ThemeEditor", [](QWidget* parent) -> QWidget*
		{
			return new ThemeEditorWidget(parent);
		});
}
void MainWindow::setupConnections()
{
	QObject::connect(&EventBus::instance(), &EventBus::networkStatusChanged, this, [](const QString& exchange, const QString& status)
		{
			qDebug() << "[Status BUS]" << exchange << "->" << status;

			// 💡 Пометка на будущее: когда сделаешь красивый статус-бар внизу окна,
			// ты прямо отсюда будешь писать: m_statusBar->setText(status);
		});
	QObject::connect(&EventBus::instance(), &EventBus::deepHistoryReady, this, [](const QString& exchange, const QString& symbol, const std::vector<Candle>& candles)
			{
				qDebug() << "[Data BUS History]" << exchange << symbol << "Count: " << candles.size();
			});

	QObject::connect(&EventBus::instance(), &EventBus::liveCandleReceived, this, [](const QString& exchange, const QString& symbol, const Candle& candle)
		{
			qDebug() << "[Data BUS Live]" << exchange << symbol << "New Tick Price: " << candle.close;
		});
}
