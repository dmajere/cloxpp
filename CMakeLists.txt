cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_CXX_STANDARD 17)
enable_testing()

# set(This cpploxlib)
project(cloxpp)

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


# add_library(${This} STATIC ${Sources})
# add_subdirectory(src/Lox)

# find_package(gflags REQUIRED)
# target_link_libraries(${This} ${GFLAGS_LIBRARIES} lang)

# add_subdirectory(test)

add_executable(cloxpp ${Sources})
# target_link_libraries(cpplox cpploxlib)
