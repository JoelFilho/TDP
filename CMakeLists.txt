cmake_minimum_required(VERSION 3.10.0)

project(tdp VERSION 0.0.1)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

find_package(Threads REQUIRED)

target_include_directories(${PROJECT_NAME} INTERFACE 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

target_link_libraries(${PROJECT_NAME} INTERFACE Threads::Threads)

option(BUILD_EXAMPLES "Build example code" ON)

if(${BUILD_EXAMPLES})
  add_subdirectory(examples)
endif()