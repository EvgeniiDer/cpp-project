# QuantumTrader — структура, отложенные решения, дорожная карта

## 1. Структура проекта

```
cpp-project/
├── CMakeLists.txt
├── CMakePresets.json
├── reading_list.md
├── docs/
│   ├── ROADMAP.md           этот файл
│   └── diagrams/            Diagram.pdf, Diagrams.png, plantuml_export.puml
├── external/                ads, catboost, simdjson
├── ml/
│   ├── train_catboost.py
│   ├── train.sh
│   ├── models/              catboost_model.cbm
│   └── catboost_info/       логи обучения CatBoost
├── tests/                   Qt Test (add_qt_test в CMakeLists.txt)
│   ├── AggressiveIcebergDetectorTest.cpp
│   ├── DetectorMathTest.cpp
│   └── MarketEventLogTest.cpp
└── src/
    ├── main.cpp
    ├── README.md
    ├── core/
    │   ├── events/          EventBus
    │   ├── managers/        LinkManager, MarketDataManager
    │   ├── marketdata/
    │   │   ├── DatasetTestCollector
    │   │   ├── OrderBookCsvExporter
    │   │   ├── OrderBookHistoryManager
    │   │   ├── detectors/   IOrderBookFeatureDetector.h, DetectorMath.h,
    │   │   │                SpoofDetector, IcebergDetector,
    │   │   │                AggressiveIcebergDetector
    │   │   ├── log/         MarketEventLog
    │   │   └── types/       OrderBookDataTypes.h, OrderBookFeatureRow.h
    │   ├── ml/              MarketPredictor
    │   ├── models/          Candle.h, ChartDependencies.h
    │   ├── network/
    │   │   ├── common/      IExchangeConnector.h, NetworkTypes.h
    │   │   └── bybit/       BybitConnector, ByBitParser,
    │   │                    WebSocketPool, WebSocketWorker
    │   └── storage/         CandleHistoryManager
    ├── ui/
    │   ├── frame/           MainWindow, WindowManager, ActionManager
    │   ├── components/      SymbolLineEdit, TimeFrameWidget,
    │   │   └── theme_editor/  CssHighlighter, QssEditor, ThemeEditorWidget
    │   └── charts/
    │       ├── FastChart, ChartContainer, ChartTypes.h
    │       ├── layers/        Axis, Candle, Crosshair, Grid, IChartLayer.h
    │       ├── orderbook/     Base, Classic, Cluster (отключён),
    │       │                  Container, Model, RenderTypes.h
    │       └── timeandsales/  Container, Widget, DataTypes.h
    ├── resources/           resources.qrc, styles/dark_theme.qss
    └── utils/               AppInitializer, ThemeManager, MockDataGenerator.h
```

В планах по src/README.md, но в коде пока нет: core/indicators,
core/storage/BinaryStorage.h, core/network/finam, ui/watchlist, ui/portfolio.

## 2. Отложенные решения

### Журнал событий (MarketEventLog)
- [ ] Срок хранения (kEventLogRetentionMs) не выбран (MarketEventLog.h:38)
- [ ] Файл журнала только дописывается, не чистится (удаление идёт только из памяти).
      Решить: перезаписывать при загрузке или чистить периодически
- [x] Неизвестный тип события в файле раньше превращался в Sweep. Исправлено:
      typeFromString возвращает std::optional, loadFromDisk пропускает такую строку
      (тест load_unknownType_behavesAsDecided)
- [ ] Устаревание при загрузке: комментарий говорит "от текущего момента",
      код считает от timestamp последней записи (loadFromDisk)
- [x] Потеря точности цены в файле: QTextStream писал 6 значащих цифр,
      67123.45 -> 67123.5. Исправлено: QString::number(price, 'g', kPricePrecision),
      kPricePrecision = 17 (тест record_price_keepsPrecisionAfterReload)
- [ ] События, пришедшие не по порядку времени (меньший timestamp после большего),
      не вытесняются: evictStale смотрит только на начало деки. Сейчас такое событие
      остаётся и считается в countNear. Решить: игнорировать или вытеснять сразу.
      Зафиксировать тестом record_outOfOrderEvent_behavesAsDecided
- [ ] magnitude события: "насколько большое, пока в разработке" (MarketEventLog.h:17)
- [ ] Мелочи: <Qfile>/<QTextstream> с неверным регистром, нет <cstdint> для uint8_t

### Время (timestamp)
- [x] DatasetTestCollector: recordState получает snapshot.timestamp (время Bybit,
      поле ts), а не QDateTime::currentMSecsSinceEpoch()
- [x] OrderBookModel::applyDelta обновляет m_snapshot.timestamp = delta.timestamp
      (раньше после дельт в модели оставалось время последнего полного снимка)
- [ ] Правило: парсер каждой биржи обязан отдавать время в миллисекундах UTC
      (для MOEX проверить единицы и часовой пояс: время может приходить по Москве
      без указания пояса). Остальной код про биржи не знает
- [ ] Допущение: время идёт вперёд в одном потоке стакана, нарушение порядка не
      обрабатывается. Учесть при подключении через WebSocketPool (несколько
      соединений) и при смене источника времени
- [ ] DatasetTestCollector.cpp: убрать лишний #include<QDateTime>

### Детектор AggressiveIcebergDetector
- [ ] restedQty: остаток агрессора ложится на ПРОТИВОПОЛОЖНУЮ сторону
      (код закомментирован в processSide)
- [ ] Переименовать tickTolerance в tickSize
- [ ] Имя параметра в .h (tolerance) и в .cpp (priceEpsilon) не совпадает

### Прочие пометки из кода
- [ ] SymbolLineEdit.cpp:54: setMaxVisibleItems(13) вывести в настройки
- [ ] TimeFrameWidget.cpp:8: убрать магические цифры
- [ ] ChartTypes.h:33: initialCandleCount = 500 сделать настройкой пользователя
- [ ] FastChart.cpp:24: использовать указатель m_gridLayer вместо m_layers[0]
- [ ] OrderBookCluster.h/.cpp: временно отключён, будет переделан
- [ ] train_catboost.py:46: заполнить реальными часами сессий MOEX

## 3. Дорожная карта

### Тесты (Qt Test), снизу вверх
- [x] 1. Каркас: tests/, add_qt_test, пустой тест
- [x] 2. DetectorMath: sumTradedQtyAtPriceByAggressor
- [ ] 3. MarketEventLog: countNear, запись/чтение файла, evictStale, грязный файл
- [ ] 4. AggressiveIcebergDetector: 10 тестов (+ ask repeatCount, порог 90%)
- [ ] 5. Подключить детектор в OrderBookHistoryManager (#include, push_back
      в конструкторе, MarketEventLog в watch(), m_eventLog.get() 8-м элементом
      в DetectionContext)
- [ ] 6. Проверить в живом приложении через DatasetTestCollector
      (bidSweepLevels/askSweepLevels в CSV)

### Этап 4: Order Flow / ML (по src/README.md)
- [x] Интерфейс IOrderBookFeatureDetector
- [x] SpoofDetector, IcebergDetector
- [x] OrderBookHistoryManager работает через вектор детекторов
- [x] AggressiveIcebergDetector (в src/README.md ещё помечен как "в планах")
- [ ] OrderBookCsvExporter: экспорт полей Iceberg и sweep-полей
- [ ] train_catboost.py: убрать классификатор SPOOF_SIDES, обучать на
      price-movement target, сигналы использовать как input features
- [ ] ALOR/MOEX-коннектор (JWT, часы сессий)

### Этапы 1-3 (по src/README.md)
- Этап 1: умная сетка окон (слева направо, перенос строкой, потом вкладки),
  контекстное меню графика
- Этап 2: движок таймфреймов (агрегация свечей), индикаторы через TA-Lib,
  скрипты Lua (sol2)
- Этап 3: QSS-редактор в ADS, локальные стили по ID окна, цветовые группы
  связи (Red/Blue/Green), привязка к DockArea

### Идеи
- LOD при отзуме: на один пиксель одна линия High-Low (таймфрейм не меняется)
- Свечи как компактные POD-структуры в бинарных буферах (std::byte),
  агрегация 5 минуток из минуток прямо в памяти
- Масштабирование: правый край остаётся у границы графика, левый смещается
- Пользовательские индикаторы: Lua, Compute Shaders или визуальные ноды

---
Статусы взяты из src/README.md и из работы над тестами; README мог устареть,
сверяйте с кодом.
