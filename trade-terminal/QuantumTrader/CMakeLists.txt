cmake_minimum_required(VERSION 3.16)

# Имя проекта и версия
project(QuantumTrader VERSION 1.0 LANGUAGES CXX)

# Стандарт C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую генерацию MOC, UIC, RCC для Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "B:/Qt/6.10.1/msvc2022_64") 
# Находим пакеты Qt6

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

# --- ПОИСК ФАЙЛОВ ---
# Мы ищем ВСЕ файлы внутри папки src (рекурсивно).
# CONFIGURE_DEPENDS заставит Visual Studio увидеть новые файлы, если вы их добавите.
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h"
    "src/*.ui"
    "src/*.qrc"
)
# --- СОЗДАНИЕ ПРИЛОЖЕНИЯ ---
# Создаем исполняемый файл из всех найденных исходников
add_executable(QuantumTrader ${PROJECT_SOURCES} "src/ui/charts/FastChart.h")

# --- НАСТРОЙКА INCLUDE ПУТЕЙ ---
# Самый важный момент для вашей структуры!
# Это позволяет писать #include "core/models/Candle.h" вместо "../core/models/Candle.h"
target_include_directories(QuantumTrader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- ЛИНКОВКА БИБЛИОТЕК ---
target_link_libraries(QuantumTrader PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
)