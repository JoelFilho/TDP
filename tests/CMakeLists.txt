set(DOCTEST_NO_INSTALL ON  CACHE BOOL "Do not install doctest with TDP by default")
set(DOCTEST_WITH_TESTS OFF CACHE BOOL "Do not install build doctest's tests and examples")
add_subdirectory(doctest)

file(GLOB TEST_FILES CONFIGURE_DEPENDS "*.cpp")

set(TEST_TARGET "${PROJECT_NAME}_tests")

add_executable(${TEST_TARGET} ${TEST_FILES})

target_link_libraries(${TEST_TARGET} PRIVATE
  ${PROJECT_NAME}
  doctest_with_main
)

# CTest Integration
include(CTest)
include(doctest/scripts/cmake/doctest.cmake)

doctest_discover_tests(${TEST_TARGET})