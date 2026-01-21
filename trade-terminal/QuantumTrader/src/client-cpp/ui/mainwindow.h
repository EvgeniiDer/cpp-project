#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class FastChart;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();
private:
	FastChart* m_chartWidget;
};

#endif // MAINWINDOW_H