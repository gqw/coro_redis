cmake_minimum_required (VERSION 3.21)

project(coro_redis)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}/bin/)
set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_HOME_DIRECTORY}/include ${CMAKE_HOME_DIRECTORY}/examples/utils )

# add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/100)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/200)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/300)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/301)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/302)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/303)
 