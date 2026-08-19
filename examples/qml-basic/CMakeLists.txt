find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick QuickControls2)

qt_add_executable(matomoqt_example_qml_basic main.cpp)

set_target_properties(matomoqt_example_qml_basic PROPERTIES AUTOMOC ON)

qt_add_qml_module(matomoqt_example_qml_basic
    URI MatomoQtExamples.QmlBasic
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/SettingsPanel.qml
)

target_compile_features(matomoqt_example_qml_basic PRIVATE cxx_std_20)

set_target_properties(matomoqt_example_qml_basic PROPERTIES
    CXX_EXTENSIONS OFF
    OUTPUT_NAME matomoqt-example-qml-basic
)

target_link_libraries(matomoqt_example_qml_basic
    PRIVATE
        MatomoQt::Qml
        MatomoQtQmlPlugin
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
)
