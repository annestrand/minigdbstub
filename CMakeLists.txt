cmake_minimum_required(VERSION 3.10)
project(minigdbstub)

# GoogleTest
set(CMAKE_CXX_STANDARD 11)
find_package(GTest REQUIRED)

set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_executable(minigdbstub_tests)
target_sources(minigdbstub_tests PRIVATE
    ${TESTS_DIR}/test_breakpoint.cpp
    ${TESTS_DIR}/test_mem.cpp
    ${TESTS_DIR}/test_recv.cpp
    ${TESTS_DIR}/test_regs.cpp
    ${TESTS_DIR}/test_send.cpp
)
target_include_directories(minigdbstub_tests PRIVATE ${CMAKE_SOURCE_DIR})
if (MSVC)
    target_compile_options(minigdbstub_tests PRIVATE "/Wall")
else()
    target_compile_options(minigdbstub_tests PRIVATE "-Wall")
    target_compile_options(minigdbstub_tests PRIVATE "-pedantic")
    # Certain tests dont use all of the stub's functions - silence this
    target_compile_options(minigdbstub_tests PRIVATE "-Wno-unused-function")
endif()
target_link_libraries(minigdbstub_tests
    GTest::GTest
    GTest::Main
)
