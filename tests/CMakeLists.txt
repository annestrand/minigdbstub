cmake_minimum_required(VERSION 3.10)

project(minigdbstub_tests)

# Include minigdbstub header
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/tests)
add_compile_options(-Wall)
add_compile_options(-pedantic)
add_definitions(-DMINIGDBSTUB_DEBUG)

# Tests
add_executable(test_recv test_recv.cpp)
target_link_libraries(test_recv ${CONAN_LIBS})

add_executable(test_send test_send.cpp)
target_link_libraries(test_send ${CONAN_LIBS})

add_executable(test_regs test_regs.cpp)
target_link_libraries(test_regs ${CONAN_LIBS})

add_executable(test_mem test_mem.cpp)
target_link_libraries(test_mem ${CONAN_LIBS})