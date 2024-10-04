cmake_minimum_required(VERSION 3.5)

project(Socket LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(LIB_NAME Socket)
file(GLOB LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if (NOT SOURCES)
    message(FATAL_ERROR "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src/")
endif()

message(STATUS ${SOURCES})

add_library(${LIB_NAME} SHARED ${SOURCES})
target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
