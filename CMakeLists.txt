cmake_minimum_required(VERSION 2.8.4)
project(Person)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES person.c)
add_executable(Person ${SOURCE_FILES})