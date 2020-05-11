file(GLOB EXAMPLES CONFIGURE_DEPENDS "*.cpp")

if(MSVC)
  set(WARNINGS /W4 /WX /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  set(WARNINGS -Wall -Wextra -Werror)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(WARNINGS -Wall -Wextra -Werror)
else()
  message(AUTHOR_WARNING "Warnings for compiling examples not set.")
endif()

foreach(EXAMPLE_SOURCE IN ITEMS ${EXAMPLES})
  # Get executable name from source name
  get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)

  # Create the executable, link with the library and set the compiler flags
  add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
  target_link_libraries(${EXAMPLE_NAME} ${PROJECT_NAME})
  
  # Set warnings
  target_compile_options(${EXAMPLE_NAME} PRIVATE ${WARNINGS})
endforeach()