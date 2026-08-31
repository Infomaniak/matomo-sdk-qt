cmake_minimum_required(VERSION 3.16)

project(MatomoQtConsumer LANGUAGES CXX)

find_package(MatomoQt CONFIG REQUIRED)

add_executable(matomoqt_consumer main.cpp)
target_link_libraries(matomoqt_consumer PRIVATE MatomoQt::Core)

if(TARGET MatomoQt::Qml)
    add_executable(matomoqt_qml_consumer qml_main.cpp)
    target_link_libraries(matomoqt_qml_consumer
        PRIVATE
            MatomoQt::Qml
            MatomoQt::QmlPlugin
            Qt6::Qml
    )
endif()
