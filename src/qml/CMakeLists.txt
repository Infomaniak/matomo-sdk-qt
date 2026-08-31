add_library(MatomoQtQml)
add_library(MatomoQt::Qml ALIAS MatomoQtQml)

set_target_properties(MatomoQtQml PROPERTIES AUTOMOC ON)

qt_add_qml_module(MatomoQtQml
    URI MatomoQt
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/MatomoQt"
    PLUGIN_TARGET MatomoQtQmlPlugin
    OUTPUT_TARGETS matomoqt_qml_internal_targets
    SOURCES
        QmlMatomoTracker.h
        QmlMatomoTracker.cpp
)
add_library(MatomoQt::QmlPlugin ALIAS MatomoQtQmlPlugin)

# qt_add_qml_module() generates internal object-library targets (e.g.
# MatomoQtQml_resources_1, MatomoQtQmlPlugin_init) and links them INTERFACE
# into MatomoQtQml and MatomoQtQmlPlugin. CMake's install(EXPORT) requires
# every target in the link closure to be in an export set, so we capture them
# via OUTPUT_TARGETS and partition them by the _is_qt_plugin_init_target
# property (the marker Qt uses to distinguish _init from _resources_* targets).
# The _init target belongs to the plugin; the _resources_* targets belong to
# the backing library.
set(matomoqt_qml_backing_export_targets "")
set(matomoqt_qml_plugin_export_targets "")
foreach(_t IN LISTS matomoqt_qml_internal_targets)
    get_target_property(_is_init "${_t}" _is_qt_plugin_init_target)
    if(_is_init)
        list(APPEND matomoqt_qml_plugin_export_targets "${_t}")
    else()
        list(APPEND matomoqt_qml_backing_export_targets "${_t}")
    endif()
endforeach()

set_target_properties(MatomoQtQmlPlugin PROPERTIES
    EXPORT_NAME QmlPlugin
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

if(MATOMOQT_INSTALL)
    install(
        TARGETS MatomoQtQml ${matomoqt_qml_backing_export_targets}
        EXPORT MatomoQtTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    install(
        TARGETS MatomoQtQmlPlugin ${matomoqt_qml_plugin_export_targets}
        EXPORT MatomoQtTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
    )

    get_target_property(matomoqt_qml_module_output_dir MatomoQtQml QT_QML_MODULE_OUTPUT_DIRECTORY)
    if(matomoqt_qml_module_output_dir)
        install(
            DIRECTORY "${matomoqt_qml_module_output_dir}/"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        )
    endif()
endif()
