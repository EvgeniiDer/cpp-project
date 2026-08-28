# QuantumTrader Pro — сводка сессии (для нового чата)

## Проект
QuantumTrader Pro — торговый терминал на C++20 / Qt6 / OpenGL, подключается к Bybit по WebSocket + REST. Диплом Евгения, защита прошла. Структура: `core/` (бизнес-логика, без Qt-виджетов) и `ui/` (Qt-виджеты), связаны через `EventBus` (Qt-сигналы). Папка проекта: `B:\workspace\www.github.com\cpp-project\trade-terminal\QuantumTrader`.

## Что сделали в этой сессии (аналитический модуль под CatBoost)

Цель — собирать и размечать признаки стакана для обучения CatBoost-модели детекции спуфинга/аномальной активности.

**1. `TradeTick` перенесён** из `ui/charts/timeandsales/TimeAndSalesDataTypes.h` в `core/marketdata/OrderBookDataTypes.h` — убрали обратную зависимость core→ui. Убрано поле `time` (QString) — форматирование строки теперь делается на лету в `TimeAndSalesWidget::paintEvent` через `QDateTime::fromMSecsSinceEpoch(tick.timestampMs)`.

**2. Пайплайн `tickSize` с биржи**: `ByBitParser::parseInstrumentTickSizes` (парсит `priceFilter.tickSize` из того же ответа `/v5/market/instruments-info`) → `EventBus::instrumentTickSizesLoaded` → `MarketDataManager` (кэш `m_tickSizeCache` + `getTickSize()`). `BybitConnector::fetchAvailableSymbols` сохраняет `reply->readAll()` в `raw` один раз и парсит и символы, и tickSize из одних байт.

**3. `OrderBookHistoryManager`** (`core/marketdata/`) — главный класс:
   - многоуровневый order flow imbalance (`calcDepthImbalance`, по `m_detpthLevels` уровням, не только L1) + сглаживание SMA3 по последним 3 снэпшотам
   - детект спуфинга: если объём на лучшей цене упал между снэпшотами, сверяется с реальными сделками из буфера `m_pendingTrades` (наполняется через подписку на `EventBus::tradeReceived`) — `bidSpoofDrop/askSpoofDrop` (абсолютное необъяснённое падение) и `bidFakeRatio/askFakeRatio` (доля от 0 до 1, а не жёсткая метка)
   - скользящее FIFO-окно на `std::deque<OrderBookFeatureRow>`, вытеснение старых строк по времени (`evicOldRows`, cutoff = currentTs - m_retentionMs)
   - константы окна вынесены в `namespace OrderBookConfig` (`kOneWeekMs`, `kTwoWeeksMs`, `kThreeMonthsMs` и т.д.)
   - `watch(exchange, symbol, tickSize)` — настраивает, какой инструмент слушать и какой tickSize использовать как допуск сравнения цен (вместо `kPriceEpsilon*100` заглушки)
   - `takeHistory()` — теперь `const`, просто снимок окна, не отбирает и не чистит буфer

**4. `OrderBookCsvExporter`** (отдельный класс, не в `OrderBookHistoryManager` — чтобы не смешивать сбор фичей и сериализацию, SRP): `exportToCsv(rows, filePath, horizonMs)`. Таргет — упрощённый Triple Barrier: только time-barrier (бинарный поиск `std::lower_bound` по timestamp вперёд на `horizonMs`, сравнение midPrice). Полноценный Triple Barrier (TP/SL через ATR) — в планах, не реализован.

**5. `DatasetTestCollector`** (`core/marketdata/`) — тестовый живой сборщик: подписывается на `EventBus::orderBookReceived` + `EventBus::instrumentTickSizesLoaded`, мержит дельты через `OrderBookModel` (переиспользован из `ui/charts/orderbook/`, он сам по себе без QWidget-зависимостей), сам вызывает `subscribeToStream` и на `orderbook`, и на `trades` (без этого `fakeRatio` был бы всегда 0 или 1 — без сделок нечем объяснить падение объёма). Раз в минуту экспортирует CSV через таймер. Подключён в `MainWindow.cpp` после `connectTo("Bybit")`. **Проверено вживую на BTCUSDT — работает, CSV с адекватными признаками.**

## Известные открытые задачи / TODO
- Полноценный Triple Barrier (TP/SL через ATR), а не только time-horizon
- Time&sales-агрегация по фикс-окну (1 сек: buy/sell volume, VWAP, print clustering)
- Order lifetime по уровням цены (cancel-to-fill ratio, quote flickering)
- Архитектура из двух моделей: detector (3 мес. окно) + entry (2 нед. окно) — сейчас есть только один универсальный `OrderBookHistoryManager`, под два коллектора с разными `retentionMs` его ещё не развели по факту в живом коде (только обсуждали как это сделать)
- ALOR как вторая биржа для кросс-venue признаков — не начато
- Модель нужно периодически переобучать с нуля на текущем FIFO-окне (не дообучать инкрементально через `init_model` — это не даст эффекта "забывания")
- `DatasetTestCollector` сейчас на тестовом окне 5 минут — для боевого режима поменять на реальные 2 недели/3 месяца и убрать нумерацию файлов (сейчас каждый экспорт создаёт новый `dataset_test_N.csv`)

## Git / GitHub
Репозиторий `https://github.com/EvgeniiDer/cpp-project` — это монорепо, там ещё лежит несвязанный C#-проект (`Arbitrage/GetTokenFromTelegram`). Был инцидент: пришлось force-push (`git push origin main --force`) поверх удалённой ветки, чтобы успеть до защиты — удалённая история на тот момент перезаписана локальной. Также убирали из git большой файл `external/catboost/catboost-linux-x86_64-1.2.10` (274 МБ, превышал лимит GitHub 100 МБ) через `git rm --cached` + `.gitignore` + `commit --amend`. **После защиты нужно разобраться с этим репозиторием** — разнести монорепо, проверить, что ничего важного из C#-проекта не потерялось при force-push.

## Презентация
`speech_diploma_v2.md` — обновлённая речь (9 слайдов с учётом ML-модуля, C++20 вместо C++17, новые вопросы-ответы). `QuantumTrader_Defense.pptx` — сгенерированная колода (9 слайдов, тёмно-сливовый фон + янтарный акцент — специально не тёмно-синий/бирюзовый, т.к. у одногруппника такая же тема). Оба файла в корне проекта. Старые `slide-*.jpg`/`slide2-*.jpg` удалены.

## Как обращаться
Пользователя зовут Евгений, просил звать меня Клод. Предпочитает: не переписывать код за него без спроса — давать код текстом в чате, он переносит и правит сам. Пишет с матом, когда паникует (было под давлением дедлайна защиты) — по факту всё разрешилось, защита прошла.
