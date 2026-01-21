#include"ui/mainwindow.h"
#include<qapplication.h>


int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	MainWindow mainWindow;

	mainWindow.show();
	return app.exec();
}