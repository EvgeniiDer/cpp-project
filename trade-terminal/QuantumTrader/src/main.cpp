#include<QApplication>
#include<qstring>
#include"ui/frame/MainWindow.h"
#include"utils/AppInitializer.h"

int main(int argc, char* argv[])
{
    AppInitializer::setupGraphiccs();

    QApplication a(argc, argv);
    MainWindow window;
    window.show();
    return a.exec();
}