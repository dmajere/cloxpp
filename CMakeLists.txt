cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_CXX_STANDARD 17)
enable_testing()

set(This cloxpp)
project(${This})

find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
if (CMAKE_CXX_CPPCHECK)
    list(
        APPEND CMAKE_CXX_CPPCHECK 
            "--enable=warning"
            "--inconclusive"
            "--force" 
            "--inline-suppr"
            "--suppressions-list=${CMAKE_SOURCE_DIR}/CppCheckSuppressions.txt"
    )
endif()

set(Sources 
${CMAKE_CURRENT_SOURCE_DIR}/src/cloxpp.cpp
)

add_subdirectory(src/compiler)

find_package(gflags REQUIRED)
find_package(folly CONFIG REQUIRED)
include_directories(${FOLLY_INCLUDE_DIR})

add_executable(${This} ${Sources})
target_link_libraries(${This} compiler ${GFLAGS_LIBRARIES} ${FOLLY_LIBRARIES} )
