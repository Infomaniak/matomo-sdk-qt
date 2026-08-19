if(NOT MATOMOQT_BUILD_QML)
    message(FATAL_ERROR "MATOMOQT_BUILD_EXAMPLES requires MATOMOQT_BUILD_QML=ON (the QML example needs the MatomoQt QML module).")
endif()

add_subdirectory(qml-basic)
