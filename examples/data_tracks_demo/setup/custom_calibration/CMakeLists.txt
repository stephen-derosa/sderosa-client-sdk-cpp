cmake_minimum_required(VERSION 3.20)
set(project "custom_calibration")
project(${project})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3")

include(FetchContent)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../../../cmake")
include(spdlog) # defines spdlog::spdlog and _SPDLOG_ACTIVE_LEVEL

include_directories(../../../include)
include_directories(../../../../examples/data_tracks_demo/include)
include_directories(../../../../src)
include_directories(../../../../include)
link_directories(../../../build)
link_libraries(libSCServo.a)

file(GLOB hdrs *h)
set(extra_srs
  ../../../../examples/data_tracks_demo/src/pan_tilt_controller.cpp
  ../../../../src/logging.cpp
)

# custom calibration executable
add_executable(${project}
  ${hdrs}
  custom_calibration.cpp
  ${extra_srs}
)
target_link_libraries(${project} PRIVATE spdlog::spdlog)
target_compile_definitions(${project} PRIVATE SPDLOG_ACTIVE_LEVEL=${_SPDLOG_ACTIVE_LEVEL})

# keyboard velocity controller executable
add_executable(KeyboardController
  ${hdrs}
  keyboard_controller.cpp
  ${extra_srs}
)
target_link_libraries(KeyboardController PRIVATE spdlog::spdlog)
target_compile_definitions(KeyboardController PRIVATE SPDLOG_ACTIVE_LEVEL=${_SPDLOG_ACTIVE_LEVEL})