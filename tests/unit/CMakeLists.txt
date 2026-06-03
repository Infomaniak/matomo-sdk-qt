find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_tracker tst_tracker.cpp)

set_target_properties(tst_tracker PROPERTIES AUTOMOC ON)

target_link_libraries(tst_tracker
    PRIVATE
        MatomoQt::Core
        Qt6::Test
)

add_test(NAME tst_tracker COMMAND tst_tracker)
