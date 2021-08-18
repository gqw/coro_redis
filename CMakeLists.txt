cmake_minimum_required (VERSION 3.6)

project(coro_redis)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}/bin/)

include_directories(${CMAKE_HOME_DIRECTORY}/include)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/100)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/200)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/300)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/301)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/302)
