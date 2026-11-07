# Phase 3 SFML build (graphical version).
#
# Needs SFML 2.5 or newer 2.x (tested with SFML 2.6.2, MinGW 64-bit, gcc 13.1).
# If CMake says "Could not find SFML", tell it where SFML is:
#   cmake -S . -B build -G "MinGW Makefiles" -DSFML_DIR=C:/SFML-2.6.2/lib/cmake/SFML
# (SFML_DIR is the folder that contains SFMLConfig.cmake)
#
# The engine is in ../core. main.cpp includes it with a relative path, so no
# include directory has to be set.
cmake_minimum_required(VERSION 3.16)
project(TrafficSimPhase3SFML LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# use the usual install folder if the evaluator did not give SFML_DIR
if(NOT SFML_DIR AND EXISTS "C:/SFML-2.6.2/lib/cmake/SFML")
    set(SFML_DIR "C:/SFML-2.6.2/lib/cmake/SFML")
endif()

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(traffic_sfml main.cpp)
target_link_libraries(traffic_sfml PRIVATE sfml-graphics sfml-window sfml-system)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(traffic_sfml PRIVATE -Wall -Wextra)
endif()

# Windows looks for the SFML .dll files next to the .exe. Without them the program
# does not start at all. So they are copied after every build.
# SFML_DIR is <root>/lib/cmake/SFML, so the root is three folders up.
get_filename_component(SFML_ROOT "${SFML_DIR}/../../.." ABSOLUTE)
foreach(dll sfml-graphics-2 sfml-window-2 sfml-system-2)
    if(EXISTS "${SFML_ROOT}/bin/${dll}.dll")
        add_custom_command(TARGET traffic_sfml POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SFML_ROOT}/bin/${dll}.dll" "$<TARGET_FILE_DIR:traffic_sfml>")
    endif()
endforeach()

# the timeline/key logic is tested WITHOUT a window, so these tests need no SFML library
enable_testing()
foreach(t test_view test_stress test_liveview test_roadsim test_roadstress)
    add_executable(${t} ../tests/${t}.cpp)
    add_test(NAME ${t} COMMAND ${t})
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${t} PRIVATE -Wall -Wextra)
    endif()
endforeach()

# test_stress and test_roadstress count every new and delete. With MinGW the C++ library is a .dll that cannot see
# the counting new/delete of the program, so the counts would be wrong in optimised builds
# (this project builds in Release by default). Linking the library statically fixes that.
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(test_stress PRIVATE -static)
    target_link_options(test_roadstress PRIVATE -static)
endif()
