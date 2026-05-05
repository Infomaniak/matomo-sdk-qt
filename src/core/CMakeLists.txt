add_library(MatomoQtCore)
add_library(MatomoQt::Core ALIAS MatomoQtCore)

target_sources(MatomoQtCore
    PRIVATE
        Tracker.cpp
        TrackerConfig.cpp
        Version.cpp
        "${PROJECT_SOURCE_DIR}/include/MatomoQt/RequestResult.h"
        "${PROJECT_SOURCE_DIR}/include/MatomoQt/Tracker.h"
        "${PROJECT_SOURCE_DIR}/include/MatomoQt/TrackerConfig.h"
        "${PROJECT_SOURCE_DIR}/include/MatomoQt/Version.h"
)

target_compile_features(MatomoQtCore PUBLIC cxx_std_20)

set_target_properties(MatomoQtCore PROPERTIES
    AUTOMOC ON
    CXX_EXTENSIONS OFF
    EXPORT_NAME Core
    OUTPUT_NAME MatomoQtCore
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
)

include(GenerateExportHeader)
generate_export_header(MatomoQtCore
    BASE_NAME MATOMOQT_CORE
    EXPORT_FILE_NAME "${PROJECT_BINARY_DIR}/include/MatomoQt/Export.h"
)

get_target_property(matomoqt_core_type MatomoQtCore TYPE)
if(matomoqt_core_type STREQUAL "STATIC_LIBRARY")
    target_compile_definitions(MatomoQtCore PUBLIC MATOMOQT_CORE_STATIC_DEFINE)
endif()

target_compile_definitions(MatomoQtCore PRIVATE MATOMOQT_VERSION_STRING=\"${PROJECT_VERSION}\")

target_include_directories(MatomoQtCore
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
)

target_link_libraries(MatomoQtCore
    PUBLIC
        Qt6::Core
        Qt6::Network
)

if(MATOMOQT_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(MatomoQtCore PRIVATE /WX)
    else()
        target_compile_options(MatomoQtCore PRIVATE -Werror)
    endif()
endif()
