find_package(Qt6 REQUIRED COMPONENTS Network Test)

add_executable(tst_networkdispatcher tst_networkdispatcher.cpp)

set_target_properties(tst_networkdispatcher PROPERTIES AUTOMOC ON)

target_link_libraries(tst_networkdispatcher
    PRIVATE
        MatomoQt::Core
        Qt6::Network
        Qt6::Test
)

add_test(NAME tst_networkdispatcher COMMAND tst_networkdispatcher)
