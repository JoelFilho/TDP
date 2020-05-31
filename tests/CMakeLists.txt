add_subdirectory(doctest EXCLUDE_FROM_ALL)

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