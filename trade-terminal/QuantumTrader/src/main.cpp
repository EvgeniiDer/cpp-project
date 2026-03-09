#include<QApplication>
#include<cmath>
#include<algorithm> 
#include<qstring>
#include"ui/charts/FastChart.h"
#include"core/models/Candle.h" // <--- Берем Candle отсюда
#include"ui/MainWindow.h"
#include"utils/AppInitializer.h"

int main(int argc, char* argv[])
{
    AppInitializer::setupGraphiccs();

    QApplication a(argc, argv);
    MainWindow window;
    window.show();
    return a.exec();
}