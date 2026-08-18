add_library(MatomoQtQml)
add_library(MatomoQt::Qml ALIAS MatomoQtQml)

set_target_properties(MatomoQtQml PROPERTIES AUTOMOC ON)

qt_add_qml_module(MatomoQtQml
    URI MatomoQt
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/MatomoQt"
    PLUGIN_TARGET MatomoQtQmlPlugin
    SOURCES
        QmlMatomoTracker.h
        QmlMatomoTracker.cpp
)

set_target_properties(MatomoQtQmlPlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "$<TARGET_PROPERTY:MatomoQtQml,QT_QML_MODULE_OUTPUT_DIRECTORY>"
    RUNTIME_OUTPUT_DIRECTORY "$<TARGET_PROPERTY:MatomoQtQml,QT_QML_MODULE_OUTPUT_DIRECTORY>"
    ARCHIVE_OUTPUT_DIRECTORY "$<TARGET_PROPERTY:MatomoQtQml,QT_QML_MODULE_OUTPUT_DIRECTORY>"
)

target_compile_features(MatomoQtQml PUBLIC cxx_std_20)

set_target_properties(MatomoQtQml PROPERTIES
    CXX_EXTENSIONS OFF
    EXPORT_NAME Qml
    OUTPUT_NAME MatomoQtQml
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
)

target_link_libraries(MatomoQtQml
    PUBLIC
        MatomoQt::Core
    PRIVATE
        Qt6::Qml
)

if(MATOMOQT_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(MatomoQtQml PRIVATE /WX)
    else()
        target_compile_options(MatomoQtQml PRIVATE -Werror)
    endif()
endif()
