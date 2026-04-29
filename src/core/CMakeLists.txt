add_library(MatomoQtCore)
add_library(MatomoQt::Core ALIAS MatomoQtCore)

target_sources(MatomoQtCore
    PRIVATE
        Version.cpp
)

target_compile_features(MatomoQtCore PUBLIC cxx_std_20)

set_target_properties(MatomoQtCore PROPERTIES
    CXX_EXTENSIONS OFF
    EXPORT_NAME Core
    OUTPUT_NAME MatomoQtCore
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
)

get_target_property(matomoqt_core_type MatomoQtCore TYPE)
if(matomoqt_core_type STREQUAL "STATIC_LIBRARY")
    target_compile_definitions(MatomoQtCore PUBLIC MATOMOQT_CORE_STATIC)
else()
    target_compile_definitions(MatomoQtCore PRIVATE MATOMOQT_CORE_LIBRARY)
endif()

target_compile_definitions(MatomoQtCore PRIVATE MATOMOQT_VERSION_STRING=\"${PROJECT_VERSION}\")

target_include_directories(MatomoQtCore
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
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
