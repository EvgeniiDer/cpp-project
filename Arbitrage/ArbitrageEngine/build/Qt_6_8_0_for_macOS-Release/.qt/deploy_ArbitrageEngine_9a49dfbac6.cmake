include(/Users/evgenii/Project/Arbitrage/ArbitrageEngine/build/Qt_6_8_0_for_macOS-Release/.qt/QtDeploySupport.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/ArbitrageEngine-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")
_qt_internal_show_skip_runtime_deploy_message("shared Qt libs, non-bundle app"
    EXTRA_MESSAGE "Executable targets have to be app bundles to use this command on Apple platforms."
)