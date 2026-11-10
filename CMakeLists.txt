cmake_minimum_required(VERSION 3.10)
project(PeopleAreWatching CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

add_executable(people_are_watching
    src/main.cpp
    src/graphics.cpp
    src/lines.cpp
    src/circle.cpp
    src/polygon.cpp
    src/scene.cpp
)

# Works with both vcpkg/MSVC (SDL2::SDL2) and MSYS2/Linux (SDL2_INCLUDE_DIRS / SDL2_LIBRARIES)
if(TARGET SDL2::SDL2)
    target_link_libraries(people_are_watching PRIVATE SDL2::SDL2)
else()
    target_include_directories(people_are_watching PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(people_are_watching PRIVATE ${SDL2_LIBRARIES})
endif()
