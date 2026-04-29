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
    TARGETS MatomoQtCore
    EXPORT MatomoQtTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

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
