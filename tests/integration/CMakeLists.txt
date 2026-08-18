find_package(Qt6 REQUIRED COMPONENTS Network Test)

add_executable(tst_networkdispatcher tst_networkdispatcher.cpp)

set_target_properties(tst_networkdispatcher PROPERTIES AUTOMOC ON)

target_link_libraries(tst_networkdispatcher
    PRIVATE
        MatomoQt::Core
        Qt6::Network
        Qt6::Test
)

add_test(NAME tst_networkdispatcher COMMAND tst_networkdispatcher)

add_executable(tst_tracker_integration tst_tracker.cpp)

set_target_properties(tst_tracker_integration PROPERTIES AUTOMOC ON)

target_link_libraries(tst_tracker_integration
    PRIVATE
        MatomoQt::Core
        Qt6::Network
        Qt6::Test
)

add_test(NAME tst_tracker_integration COMMAND tst_tracker_integration)

if(MATOMOQT_BUILD_QML)
    find_package(Qt6 REQUIRED COMPONENTS Qml)

    add_executable(tst_qml_module tst_qml_module.cpp)

    set_target_properties(tst_qml_module PROPERTIES AUTOMOC ON)

    target_compile_definitions(tst_qml_module
        PRIVATE
            MATOMOQT_QML_IMPORT_PATH="$<TARGET_PROPERTY:MatomoQtQml,QT_QML_MODULE_OUTPUT_DIRECTORY>"
    )

    target_link_libraries(tst_qml_module
        PRIVATE
            MatomoQt::Qml
            MatomoQtQmlPlugin
            Qt6::Network
            Qt6::Qml
            Qt6::Test
    )

    add_test(NAME tst_qml_module COMMAND tst_qml_module)
endif()
