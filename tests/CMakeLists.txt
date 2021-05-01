cmake_minimum_required(VERSION 3.10)

project(minigdbstub_tests)

# Include minigdbstub header
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/tests)
add_compile_options(-Wall)
add_compile_options(-pedantic)
add_definitions(-DMINIGDBSTUB_DEBUG)

# Tests
add_executable(test_minigdbstub
    test_recv.cpp
    test_send.cpp
    test_regs.cpp
    test_mem.cpp
)
target_link_libraries(test_recv ${CONAN_LIBS})