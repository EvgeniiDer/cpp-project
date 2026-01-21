#include"mainwindow.h"
#include"charts/fastchart.h"


MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
{
	setWindowTitle("C++ UI Client - Fast Chart Example");
	resize(800, 600);
	m_chartWidget = new FastChart(this);
	setCentralWidget(m_chartWidget);
}
MainWindow::~MainWindow()
{
}