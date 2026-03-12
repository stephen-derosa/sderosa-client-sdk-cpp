cmake_minimum_required(VERSION 3.5)
project(ProgramEpromExamples)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

include_directories(../../../include)
link_directories(../../../build)
link_libraries(libSCServo.a)

add_executable(ProgramEprom ProgramEprom.cpp)
add_executable(SetLimits SetLimits.cpp)