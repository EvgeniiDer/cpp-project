cmake_minimum_required(VERSION 3.16)

# Имя проекта и версия
project(QuantumTrader VERSION 1.0 LANGUAGES CXX)

# Стандарт C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- ЖЕСТКИЙ ФИКС ДЛЯ ОШИБОК АНАЛИЗАТОРА MSVC ---
# Говорим компилятору MSVC полностью игнорировать любые предупреждения/ошибки анализа кода 
# во внешних библиотеках (включая Qt и сторонние папки)
if(MSVC)
    add_compile_options("/external:W0" "/external:I" "${CMAKE_CURRENT_SOURCE_DIR}/external")
    set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "/external:I ")
endif()

# Включаем автоматическую генерацию MOC, UIC, RCC для Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "B:/Qt/6.11.0/msvc2022_64") 

# Находим пакеты Qt6
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    OpenGLWidgets
    Network
    WebSockets
)

set(BUILD_QA_WITH_QT6 ON CACHE BOOL "Build ADS with Qt6" FORCE)
# 2. Отключаем примеры, чтобы не тратить время на их компиляцию
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable ADS examples" FORCE)

add_subdirectory("external/ads")

# --- ПОИСК ФАЙЛОВ ---
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h"
    "src/*.ui"
    "src/*.qrc"
)

# --- Путь к SIMDJSON ---
set(SIMDJSON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/external/simdjson/simdjson.cpp")

# --- СОЗДАНИЕ ПРИЛОЖЕНИЯ ---
add_executable(QuantumTrader 
    ${PROJECT_SOURCES}
    ${SIMDJSON_SOURCES}
)

# --- НАСТРОЙКА INCLUDE ПУТЕЙ ---
# Изменено на SYSTEM для внешних библиотек (ads и simdjson), 
# чтобы компилятор не проверял их на варнинги своими внутренними линтерами.
target_include_directories(QuantumTrader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(QuantumTrader SYSTEM PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ads/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/simdjson"
)

# --- ЛИНКОВКА БИБЛИОТЕК ---
target_link_libraries(QuantumTrader PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Network
    Qt6::WebSockets
    qtadvanceddocking-qt6
)