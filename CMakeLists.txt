cmake_minimum_required(VERSION 3.10)
project(MotorSync LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

if(WIN32)
    set(PLATFORM_LIBS winmm)
else()
    set(PLATFORM_LIBS "")
endif()

if(MSVC)
    add_compile_options(/O2 /W4)
else()
    add_compile_options(-O2 -Wall -Wextra)
endif()

# --- Benchmarks ---
add_executable(test_a2 benchmarks/test_a2.cpp)
target_link_libraries(test_a2 PRIVATE Threads::Threads ${PLATFORM_LIBS})

add_executable(test_b benchmarks/test_b.cpp)
target_link_libraries(test_b PRIVATE Threads::Threads ${PLATFORM_LIBS})

add_executable(test_c benchmarks/test_c.cpp)
target_link_libraries(test_c PRIVATE Threads::Threads ${PLATFORM_LIBS})


# --- examples ---
add_executable(world_model examples/world_model.cpp)
target_link_libraries(world_model PRIVATE Threads::Threads ${PLATFORM_LIBS})

# --- Run all ---
add_custom_target(run_all
    COMMAND test_a2
    COMMAND test_b
    COMMAND test_c
    DEPENDS test_a2 test_b test_c
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all benchmarks..."
)

#set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT world_model)