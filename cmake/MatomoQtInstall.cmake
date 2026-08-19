include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(NOT MATOMOQT_INSTALL)
    return()
endif()

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
    FILES "${PROJECT_BINARY_DIR}/include/MatomoQt/Export.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/MatomoQt"
)

install(
    TARGETS MatomoQtCore
    EXPORT MatomoQtTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

if(TARGET MatomoQtQml)
    install(
        TARGETS MatomoQtQml
        EXPORT MatomoQtTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
    )

    if(TARGET MatomoQtQmlPlugin)
        install(
            TARGETS MatomoQtQmlPlugin
            EXPORT MatomoQtTargets
            RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        )
    endif()

    get_target_property(matomoqt_qml_module_output_dir MatomoQtQml QT_QML_MODULE_OUTPUT_DIRECTORY)
    if(matomoqt_qml_module_output_dir)
        install(
            DIRECTORY "${matomoqt_qml_module_output_dir}/"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/qml/MatomoQt"
        )
    endif()
endif()

install(
    EXPORT MatomoQtTargets
    NAMESPACE MatomoQt::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/MatomoQt"
)

write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/MatomoQtConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/MatomoQtConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/MatomoQtConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/MatomoQt"
)

install(
    FILES
        "${PROJECT_BINARY_DIR}/MatomoQtConfig.cmake"
        "${PROJECT_BINARY_DIR}/MatomoQtConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/MatomoQt"
)
