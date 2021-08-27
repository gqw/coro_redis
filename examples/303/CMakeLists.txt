cmake_minimum_required (VERSION 3.21)

project(test_303)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/)
file(GLOB SOURCE_FILES	./src/*.h ./src/*.cpp)
file(GLOB INCLUDE_FILES	${CMAKE_SOURCE_DIR}/include/coro_redis/*)
file(GLOB UTIL_FILES	${CMAKE_HOME_DIRECTORY}/examples/utils/*.cpp
						${CMAKE_HOME_DIRECTORY}/examples/utils/*.h)

source_group("Include Files" FILES ${INCLUDE_FILES})
source_group("Util Files" FILES ${UTIL_FILES})

add_definitions(-DPROJECT_NAME="${PROJECT_NAME}")
if (WIN32)
	add_definitions(-DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601)
    add_definitions(/utf-8 /wd"4200" /wd"4996")
else()
	add_definitions(-g -O0 -Wall -fcoroutines)
endif()

find_package(spdlog CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(Libevent CONFIG REQUIRED)

add_executable(${PROJECT_NAME} ${INCLUDE_FILES} ${UTIL_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE hiredis::hiredis libevent::core
	libevent::extra spdlog::spdlog spdlog::spdlog_header_only)

