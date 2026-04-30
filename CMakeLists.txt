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

add_subdirectory(src/core)

include(cmake/MatomoQtInstall.cmake)
