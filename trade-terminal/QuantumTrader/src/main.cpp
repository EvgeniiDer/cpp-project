#include <QApplication>
#include <cmath>
#include <algorithm> 

// Подключаем файлы. НЕ КОПИРУЕМ СЮДА КОД СТРУКТУР!
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

    if (chart.candleLayer())
    {
        chart.candleLayer()->setCandles(testData);
    }

    chart.show();
    return a.exec();
}