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

add_custom_target(export_video
    COMMAND $<TARGET_FILE:people_are_watching> --export-video ${CMAKE_BINARY_DIR}/people_are_watching.mp4
    DEPENDS people_are_watching
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Rendering the documentary at 1080p / 30 fps (requires ffmpeg on PATH)"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(transform_test tests/transform_test.cpp src/transform.cpp)
    target_include_directories(transform_test PRIVATE src)
    add_test(NAME transform_test COMMAND transform_test)

    add_executable(simulation_test tests/simulation_test.cpp src/simulation.cpp src/scenario.cpp)
    target_include_directories(simulation_test PRIVATE src)
    add_test(NAME simulation_test COMMAND simulation_test)
endif()
