cmake_minimum_required(VERSION 3.16)

project(MatomoQtConsumer LANGUAGES CXX)

find_package(MatomoQt CONFIG REQUIRED)

add_executable(matomoqt_consumer main.cpp)
target_link_libraries(matomoqt_consumer PRIVATE MatomoQt::Core)
