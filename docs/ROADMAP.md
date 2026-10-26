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
      (код закомментирован в processSide, поля restedQty пока всегда 0)
- [ ] ГИПОТЕЗА проверить на данных: restedQty может быть шумом. Айсберг показывает
      в стакане только видимую часть (объём 1 или 1 + случайная добавка), реальный
      размер скрыт, поэтому признак может не нести информации.
      Как проверить: включить блок (с исправлением стороны: смотреть противоположную
      сторону, новый лучший уровень лучше прежнего), выгрузить restedQty в CSV, обучить
      CatBoost с признаком и без него, сравнить важность признака и качество.
      Не помогает: удалить закомментированный блок и поля bid/askSweepRestedQty.
      Помогает: оставить, исправить сторону и написать тесты на restedQty
- [ ] Ограничение: sweep пропускается, если съеденный уровень повторно выставили в том
      же промежутке между двумя снимками стакана. В текущем снимке на этой цене есть
      объём, поэтому fullyDepleted ложно (currentQty > kPriceEpsilon) и цепочка уровней
      обрывается, даже если сделки объясняют 100% объёма уровня.
      Идея исправления (нужно менять логику processSide): считать уровень зачищенным,
      когда traded >= prevQty * kExplainedRatioThreshold, а не когда уровень исчез;
      частичное исполнение при этом остаётся не sweep (traded < prevQty).
      Риск: можно перепутать с дозаправкой айсберга (IcebergDetector), проверить на
      живых данных (шаг 6) до изменения логики.
      Порядок: сначала тесты на текущее поведение, потом правка отдельным коммитом.
      Зафиксировать тестом detect_levelRefilledInSameFrame_behavesAsDecided
- [ ] Константы детектора одинаковы для всех инструментов: kMinSweepLevels = 2,
      kSweepScanDepth = 10, kExplainedRatioThreshold = 0.9, kRepeatZoneTicks = 3.
      Два уровня у ликвидного и у неликвидного актива значат разное.
      Порог kMinSweepLevels влияет только на журнал событий и repeatCount;
      bidSweepLevels/askSweepLevels пишутся в outRow при любом значении.
      Идеи: вынести константы в структуру настроек детектора (значения по умолчанию
      как сейчас, можно задавать для каждого инструмента); мерить объёмом
      относительно глубины стакана или расстоянием в тиках; подбирать по данным.
      Если в один датасет смешать разные активы, CatBoost не отличит их смысл
      без колонки инструмента или нормализованных признаков
- [ ] repeatCount: зона ±3.5 тика слишком широкая. При тренде следующий sweep
      оказывается в 3 тиках от прошлого и считается повтором, хотя это продолжение
      движения, а не повторный удар по тому же месту. Определять тренд автоматически
      не стоит: на таком масштабе рынок ходит пилой, правило будет ошибаться.
      Решение (согласовано): два счётчика на каждую сторону вместо одного:
      узкий (0-1 тик, удар по тому же уровню, "интересные зоны") и широкий
      (как сейчас, настойчивость одной стороны). Широкий включает узкий, поэтому
      возможен третий признак "широкий минус узкий".
      Ширины зон держать списком констант (в тиках) и считать циклом, чтобы третью
      ширину добавить одной строкой. Нужны новые поля в OrderBookFeatureRow,
      колонки в OrderBookCsvExporter, обновление теста repeatCount.
      Порядок: сначала тесты на текущий repeatCount, потом правка отдельным коммитом,
      решить до шага 5. Оценить на живых данных (распределение значений в CSV,
      важность признаков в CatBoost), лишний признак убрать
- [ ] ИДЕЯ (подумать): хранить в MarketEvent не одну цену, а диапазон снятых уровней
      (от верхнего до нижнего зачищенного). Новый sweep, который пересекается с
      диапазоном прошлого, это повторный удар по тому же месту; sweep за пределами
      диапазона это продолжение движения. Допуск по цене тогда почти не нужен
      (0-1 тик на дрожание). Цена: меняются MarketEvent, формат файла журнала,
      countNear и их тесты, поэтому решать до шага 5. Альтернатива двум счётчикам
      или дополнение к ним. Решение пока не принято
- [ ] ОШИБКА ПОРЯДКА: detect вызывает countNear ДО record, а старые события
      вытесняет только record (evictStale внутри него); countNear время не смотрит.
      Следствие: sweep видит устаревшие события. После паузы дольше срока хранения
      repeatCount всё равно >= 1 (последнее событие ещё в памяти в момент подсчёта),
      то есть repeatCount никогда не возвращается к 0 после первого sweep в зоне.
      Пример (срок 1000): события 100/200/300, sweep в 1400 даёт repeatCount 3,
      по смыслу должно быть 0; sweep в 1500 даёт 1 (событие 1400 свежее), это верно.
      Тест: detect_repeatSweep_oldEventsAreForgotten (пока ожидает 3 для 1400).
      Исправление, вариант 1 (предпочтительный): передавать в countNear текущее время
      и пропускать события старше now - retention; countNear остаётся const.
      Вариант 2: вызывать evictStale(ctx.currentTimestamp) в детекторе перед
      countNear (меняет журнал из места, где только читаем).
      Порядок: тест на текущее поведение уже есть, правка отдельным коммитом,
      после правки в тесте для 1400 ожидание сменить с 3 на 0
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
- [x] 3. MarketEventLog: countNear, запись/чтение файла, evictStale, грязный файл
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
