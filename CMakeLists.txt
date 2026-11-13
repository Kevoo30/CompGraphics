cmake_minimum_required(VERSION 3.10)
project(PeopleAreWatching CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# Picks up every .cpp in src/ automatically (new files just work, CONFIGURE_DEPENDS re-checks on build)
file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(people_are_watching ${SOURCES})

# SDL2: works with vcpkg/MSVC (SDL2::SDL2) and MSYS2/Linux (SDL2_INCLUDE_DIRS / SDL2_LIBRARIES)
if(TARGET SDL2::SDL2)
    target_link_libraries(people_are_watching PRIVATE SDL2::SDL2)
else()
    target_include_directories(people_are_watching PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(people_are_watching PRIVATE ${SDL2_LIBRARIES})
endif()

# OpenGL: OpenGL::GL on all platforms (opengl32 on Windows)
target_link_libraries(people_are_watching PRIVATE OpenGL::GL)