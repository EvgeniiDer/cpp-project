cmake_minimum_required(VERSION 3.19)
project(ArbitrageEngine LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network)

qt_standard_project_setup()

qt_add_executable(ArbitrageEngine
    main.cpp
    pricemonitor.h pricemonitor.cpp

)

target_link_libraries(ArbitrageEngine
    PRIVATE
        Qt::Core
        Qt::Network
)

include(GNUInstallDirs)

install(TARGETS ArbitrageEngine
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ArbitrageEngine
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
