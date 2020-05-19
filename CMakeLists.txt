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

# Examples and Unit testing
option(TDP_BUILD_EXAMPLES "Build TDP examples" OFF)
option(TDP_BUILD_TESTS "Build TDP unit tests" OFF)

if(${TDP_BUILD_EXAMPLES})
  add_subdirectory(examples)
endif()

if(${TDP_BUILD_TESTS})
  enable_testing()
  add_subdirectory(tests)
endif()

# Install interface

set(IS_ROOT_DIRECTORY OFF)
if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  set(IS_ROOT_DIRECTORY ON)
endif ()

option(TDP_INSTALL "Generate the TDP install interface" ${IS_ROOT_DIRECTORY})

if(TDP_INSTALL)
  include(GNUInstallDirs)

  target_include_directories(${PROJECT_NAME} INTERFACE
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-config
  )

  install(
    EXPORT ${PROJECT_NAME}-config
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )

  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()