cmake_minimum_required(VERSION 3.10)

# criterion issue - Ninja builds cant do git clones? (Using GNU Make instead)
set(CMAKE_GENERATOR "Unix Makefiles" CACHE INTERNAL "" FORCE)

project(minigdbstub)

# Build tests
add_subdirectory(external/criterion)
add_subdirectory(tests)