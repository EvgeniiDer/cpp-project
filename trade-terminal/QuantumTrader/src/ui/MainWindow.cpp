#include"MainWindow.h"
#include"charts/FastChart.h"
#include"WindowManager.h"

#include<QMenuBar>
#include<QMenu>
#include<QAction>
#include<QMdiSubWindow>
#include<QMdiArea>
#include<QDockWidget>

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent)
{

	setupUi();
	m_windowManager = new WindowManager(this);

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
	setDockOptions(QMainWindow::AnimatedDocks |
		QMainWindow::AllowNestedDocks |
		QMainWindow::AllowTabbedDocks |
		QMainWindow::GroupedDragging);

	//QOpenGLWidget* centralDummy = new QOpenGLWidget(this);
	//centralDummy->setObjectName("centralDummy");
	//setCentralWidget(centralDummy);

	QWidget* centralDummy = new QWidget(this);
	QPalette pal = centralDummy->palette();
	pal.setColor(QPalette::Window, QColor(Qt::green));
	centralDummy->setAutoFillBackground(true);
	centralDummy->setPalette(pal);
	this->setCentralWidget(centralDummy);
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
	m_propertiesDock = new QDockWidget(QObject::tr("Properties"), this);
	m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

	QWidget* emptyWidget = new QWidget();
	emptyWidget->setStyleSheet("background-color: #1e1e1e;");
	m_propertiesDock->setWidget(emptyWidget);
	addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);
}
