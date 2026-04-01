cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED LIVEKIT_SOURCE_DIR OR LIVEKIT_SOURCE_DIR STREQUAL "")
  message(FATAL_ERROR "LIVEKIT_SOURCE_DIR is required")
endif()

if(NOT DEFINED LIVEKIT_BINARY_DIR OR LIVEKIT_BINARY_DIR STREQUAL "")
  message(FATAL_ERROR "LIVEKIT_BINARY_DIR is required")
endif()

if(NOT DEFINED TEST_CONFIG OR TEST_CONFIG STREQUAL "")
  if(DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "")
    set(TEST_CONFIG "${CMAKE_BUILD_TYPE}")
  else()
    set(TEST_CONFIG "Release")
  endif()
endif()

set(_package_root "${LIVEKIT_BINARY_DIR}/package-test")
set(_install_prefix "${_package_root}/install/${TEST_CONFIG}")
set(_consumer_src_dir "${_package_root}/consumer-src")
set(_consumer_build_dir "${_package_root}/consumer-build/${TEST_CONFIG}")

file(REMOVE_RECURSE "${_install_prefix}" "${_consumer_src_dir}" "${_consumer_build_dir}")
file(MAKE_DIRECTORY "${_consumer_src_dir}" "${_consumer_build_dir}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${LIVEKIT_BINARY_DIR}" --prefix "${_install_prefix}" --config "${TEST_CONFIG}"
  RESULT_VARIABLE _install_rv
  OUTPUT_VARIABLE _install_out
  ERROR_VARIABLE _install_err
)
if(NOT _install_rv EQUAL 0)
  message(FATAL_ERROR
    "Failed to install LiveKit test package.\n"
    "stdout:\n${_install_out}\n"
    "stderr:\n${_install_err}"
  )
endif()

set(_targets_file "${_install_prefix}/lib/cmake/LiveKit/LiveKitTargets.cmake")
if(NOT EXISTS "${_targets_file}")
  message(FATAL_ERROR "Expected installed targets file at ${_targets_file}")
endif()

file(READ "${_targets_file}" _targets_contents)

set(_consumer_cmakelists "${_consumer_src_dir}/CMakeLists.txt")
set(_consumer_main "${_consumer_src_dir}/main.cpp")

file(WRITE "${_consumer_cmakelists}" [=[
cmake_minimum_required(VERSION 3.20)
project(livekit_package_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LiveKit CONFIG REQUIRED)

get_target_property(_livekit_links LiveKit::livekit INTERFACE_LINK_LIBRARIES)
if(NOT _livekit_links)
  message(FATAL_ERROR "LiveKit::livekit is missing INTERFACE_LINK_LIBRARIES")
endif()

set(_livekit_link_list ${_livekit_links})
list(FIND _livekit_link_list "spdlog::spdlog" _livekit_spdlog_index)
if(_livekit_spdlog_index EQUAL -1)
  message(FATAL_ERROR "LiveKit::livekit does not propagate spdlog::spdlog")
endif()

add_executable(livekit_consumer_smoke main.cpp)
target_link_libraries(livekit_consumer_smoke PRIVATE LiveKit::livekit)
]=])

file(WRITE "${_consumer_main}" [=[
#include <livekit/livekit.h>

int main() {
  LK_LOG_INFO("package consumer smoke test");
  return 0;
}
]=])

execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${_consumer_src_dir}" -B "${_consumer_build_dir}"
          "-DCMAKE_PREFIX_PATH=${_install_prefix}"
          "-DCMAKE_BUILD_TYPE=${TEST_CONFIG}"
  RESULT_VARIABLE _configure_rv
  OUTPUT_VARIABLE _configure_out
  ERROR_VARIABLE _configure_err
)
if(NOT _configure_rv EQUAL 0)
  message(FATAL_ERROR
    "Failed to configure LiveKit consumer smoke test.\n"
    "stdout:\n${_configure_out}\n"
    "stderr:\n${_configure_err}"
  )
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${_consumer_build_dir}" --config "${TEST_CONFIG}"
  RESULT_VARIABLE _build_rv
  OUTPUT_VARIABLE _build_out
  ERROR_VARIABLE _build_err
)
if(NOT _build_rv EQUAL 0)
  message(FATAL_ERROR
    "Failed to build LiveKit consumer smoke test.\n"
    "stdout:\n${_build_out}\n"
    "stderr:\n${_build_err}"
  )
endif()
