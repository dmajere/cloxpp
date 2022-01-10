set(This compiler)
set(Sources 
    ReadAllScanner.cpp
    ReadByOneScanner.cpp
    Parser.cpp
)

add_library(${This} ${Sources})

find_package(folly CONFIG REQUIRED)
include_directories(${FOLLY_INCLUDE_DIR})
target_link_libraries(${This} ${FOLLY_LIBRARIES})
