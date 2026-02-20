#include <QApplication>
#include <cmath>
#include <algorithm> 

#include "ui/charts/FastChart.h"
#include "core/models/Candle.h" // <--- Берем Candle отсюда

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    FastChart chart;
    chart.resize(1200, 800);
    chart.setWindowTitle("Quantum Trader - Final Test");

    std::vector<Candle> testData;
    testData.reserve(1000);

    for (int i = 0; i < 1000; ++i)
    {
        double trend = sin(i * 0.05) * 20.0;
        double noise = (rand() % 100 - 50) / 10.0;

        double o = 100.0 + trend + noise;
        double c = o + (rand() % 20 - 10) / 10.0;
        double h = std::max(o, c) + 0.5;
        double l = std::min(o, c) - 0.5;
        double v = 100 + rand() % 50;

        // Теперь emplace_back найдет конструктор в файле Candle.h
        testData.emplace_back(i, o, h, l, c, v);
    }

    if (chart.getCandleLayer())
    {
        chart.getCandleLayer()->setCandles(testData);
 //       chart.getCandleLayer() возвращает m_candleLayer которая в свою очередь в конструкторе FastChart
//        ссылаеться при помощи метода .get() на память в куче обьекта который потом мы передаем под
//        управление в vector<std::unique_ptr<IChartLayer>>m_layers 
//        std::unique_ptr<GridLayer> gridLayer = std::make_unique<GridLayer>();
//        m_gridLayer = gridLayer.get(); Корчое возвращает нам указатель на кучу где храниться этот 
//        обьект это замена если бы мы обращались бы к векторму к которому пердаеться управление как m_layers
//        [1]!!!!! Для упрощения пометка использвать в дальнешем
//        m_layers.push_back(std::move(gridLayer));

    }

    chart.show();
    return a.exec();
}