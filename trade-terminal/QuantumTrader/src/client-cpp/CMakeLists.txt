cmake_minimum_required(VERSION 3.16)
project(quantum_trader VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "B:/Qt/6.10.1/msvc2022_64")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

file(GLOB_RECURSE SOURCES
    "core/*.cpp" "core/*.h"
    "ui/*.cpp" "ui/*.h"
    "connectors/*.cpp" "connectors/*.h"
)

add_executable(quantum_trader ${SOURCES})

target_include_directories(quantum_trader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                                  ${Qt6Core_INCLUDE_DIRS}
                                                  ${Qt6Gui_INCLUDE_DIRS}
                                                  ${Qt6Widgets_INCLUDE_DIRS}
                                                  ${Qt6OpenGLWidgets_INCLUDE_DIRS})
                                        

target_link_libraries(quantum_trader PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
)
