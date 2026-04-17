#include<QApplication>
#include<cmath>
#include<algorithm> 
#include<qstring>
#include"ui/charts/FastChart.h"
#include"core/models/Candle.h" // <--- Берем Candle отсюда
#include"ui/frame/MainWindow.h"
#include"utils/AppInitializer.h"

#include"src/core/network/bybit/BybitConnector.h"
int main(int argc, char* argv[])
{
    AppInitializer::setupGraphiccs();

    QApplication a(argc, argv);
    //MainWindow window;
    //window.show();
    BybitConnector byBit;
    byBit.connect();
    return a.exec();
}