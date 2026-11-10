# Phase 3 SFML build (graphical version) and the tests.
#
# Build, then run all tests (the tests need no window):
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# Needs SFML 2.5 or newer 2.x (tested with SFML 2.6.2, MinGW 64-bit, gcc 13.1).
# If CMake says "Could not find SFML", tell it where SFML is:
#   cmake -S . -B build -G "MinGW Makefiles" -DSFML_DIR=C:/SFML-2.6.2/lib/cmake/SFML
# (SFML_DIR is the folder that contains SFMLConfig.cmake)
#
# The engine is split in two folders: ../core/include has the headers (the declarations) and
# ../core/src has the function bodies, built once as a static library (traffic_core) that the
# window links. The window's own files are split the same way: include/ has the headers
# (ViewModel, LiveView, Renderer, LiveRenderer) and src/ has their function bodies, which are
# compiled together with main.cpp. Everything includes headers with a relative path, so no
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

# the core library: every function body that was moved out of the headers
set(CORE_SOURCES
    ../core/src/ArrayPriorityQueue.cpp
    ../core/src/ArrayQueue.cpp
    ../core/src/ArrayStack.cpp
    ../core/src/AuditLog.cpp
    ../core/src/DemoScene.cpp
    ../core/src/EmergencyVehicle.cpp
    ../core/src/Graph.cpp
    ../core/src/RoadSim.cpp
    ../core/src/RouteHistory.cpp
    ../core/src/Simulator.cpp
    ../core/src/SinglyLinkedList.cpp
    ../core/src/TrafficFlow.cpp
    ../core/src/TrafficSignal.cpp)
add_library(traffic_core STATIC ${CORE_SOURCES})

add_executable(traffic_sfml main.cpp
    src/ViewModel.cpp
    src/LiveView.cpp
    src/Renderer.cpp
    src/LiveRenderer.cpp)
target_link_libraries(traffic_sfml PRIVATE traffic_core sfml-graphics sfml-window sfml-system)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(traffic_core PRIVATE -Wall -Wextra)
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

# the tests: every engine rule is checked WITHOUT a window (ViewModel and LiveView hold the screen logic,
# so they are compiled in; nothing of SFML is needed). Run them with ctest.
enable_testing()
foreach(t test_graph test_history test_traffic test_phase3 test_roadsim
          test_view test_liveview test_stress test_roadstress test_leaks)
    add_executable(${t} tests/${t}.cpp src/ViewModel.cpp src/LiveView.cpp)
    target_link_libraries(${t} PRIVATE traffic_core)
    # the tests write and delete small temporary files: they run inside the build folder
    add_test(NAME ${t} COMMAND ${t} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${t} PRIVATE -Wall -Wextra)
    endif()
endforeach()

# test_stress, test_roadstress and test_leaks count every new and delete. With MinGW the C++ library is a .dll
# that cannot see the counting new/delete of the program, so the counts would be wrong in optimised builds
# (this project builds in Release by default). Linking the library statically fixes that.
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(test_stress PRIVATE -static)
    target_link_options(test_roadstress PRIVATE -static)
    target_link_options(test_leaks PRIVATE -static)
endif()