cmake_minimum_required(VERSION 3.10)
project(MotorSync LANGUAGES CXX)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type = Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Threads
find_package(Threads REQUIRED)

# Windows-specific: winmm for timeBeginPeriod
if(WIN32)
    set(PLATFORM_LIBS winmm)
else()
    set(PLATFORM_LIBS "")
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/O2 /W4)
else()
    add_compile_options(-O2 -Wall -Wextra)
endif()

# ============================================================
# Benchmarks
# ============================================================

# Test A1: uniform load
add_executable(test_a1 benchmarks/test_a1.cpp)
target_link_libraries(test_a1 PRIVATE Threads::Threads ${PLATFORM_LIBS})

# Test A2: heterogeneous load
add_executable(test_a2 benchmarks/test_a2.cpp)
target_link_libraries(test_a2 PRIVATE Threads::Threads ${PLATFORM_LIBS})

# Test B: determinism
add_executable(test_b benchmarks/test_b.cpp)
target_link_libraries(test_b PRIVATE Threads::Threads ${PLATFORM_LIBS})

# ============================================================
# Custom target: run all benchmarks
# ============================================================
add_custom_target(run_all
    COMMAND test_a1
    COMMAND test_a2
    COMMAND test_b
    DEPENDS test_a1 test_a2 test_b
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all benchmarks..."
)
