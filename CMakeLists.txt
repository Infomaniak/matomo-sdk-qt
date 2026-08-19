cmake_minimum_required(VERSION 3.16)

project(
    MatomoQt
    VERSION 0.1.0
    DESCRIPTION "C++ QT API client SDK for the Matomo Tracking API"
    HOMEPAGE_URL "https://github.com/Infomaniak/matomo-sdk-qt"
    LANGUAGES CXX
)

include(cmake/MatomoQtOptions.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Network)
if(MATOMOQT_BUILD_QML)
    find_package(Qt6 REQUIRED COMPONENTS Qml)
endif()

if(MATOMOQT_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    else()
        message(WARNING "MATOMOQT_ENABLE_COVERAGE is only supported with GCC/Clang; ignoring.")
    endif()
endif()

add_subdirectory(src/core)
if(MATOMOQT_BUILD_QML)
    add_subdirectory(src/qml)
endif()

if(MATOMOQT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(cmake/MatomoQtInstall.cmake)
