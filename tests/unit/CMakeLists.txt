find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_tracker tst_tracker.cpp)

set_target_properties(tst_tracker PROPERTIES AUTOMOC ON)

target_link_libraries(tst_tracker
    PRIVATE
        MatomoQt::Core
        Qt6::Test
)

add_test(NAME tst_tracker COMMAND tst_tracker)

add_executable(tst_public_api tst_public_api.cpp)

target_link_libraries(tst_public_api
    PRIVATE
        MatomoQt::Core
)

add_test(NAME tst_public_api COMMAND tst_public_api)

set(matomoqt_public_headers
    ClientIdStore
    ConsentState
    ConsentStore
    CustomDimension
    Event
    PageView
    PrivacyMode
    RequestResult
    Tracker
    TrackerConfig
    Version
)

foreach(matomoqt_public_header IN LISTS matomoqt_public_headers)
    set(matomoqt_header_target "compile_public_header_${matomoqt_public_header}")
    add_executable("${matomoqt_header_target}" compile_public_header.cpp)
    target_compile_definitions("${matomoqt_header_target}"
        PRIVATE
            MATOMOQT_HEADER_UNDER_TEST=<MatomoQt/${matomoqt_public_header}.h>
    )
    target_link_libraries("${matomoqt_header_target}"
        PRIVATE
            MatomoQt::Core
    )
    add_test(NAME "${matomoqt_header_target}" COMMAND "${matomoqt_header_target}")
endforeach()
