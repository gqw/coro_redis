cmake_minimum_required (VERSION 3.21)

project(test_303)

SET(EXECUTABLE_OUTPUT_PATH		${CMAKE_HOME_DIRECTORY}/bin/)
file(GLOB SOURCE_FILES			${PROJECT_SOURCE_DIR}/src/*.h 
								${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB INCLUDE_FILES			${CMAKE_HOME_DIRECTORY}/include/coro_redis/*.hpp)
file(GLOB INCLUDE_IMPL_FILES	${CMAKE_HOME_DIRECTORY}/include/coro_redis/impl/*.ipp)
file(GLOB UTIL_FILES			${CMAKE_HOME_DIRECTORY}/examples/utils/*.cpp
								${CMAKE_HOME_DIRECTORY}/examples/utils/*.hpp)

source_group("Include Files" FILES ${INCLUDE_FILES})
source_group("Include Files/impl" FILES ${INCLUDE_IMPL_FILES})
source_group("Util Files" FILES ${UTIL_FILES})
source_group("Source Files" FILES ${SOURCE_FILES})

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

add_executable(${PROJECT_NAME} ${INCLUDE_FILES} ${INCLUDE_IMPL_FILES} ${UTIL_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE hiredis::hiredis libevent::core
	libevent::extra spdlog::spdlog spdlog::spdlog_header_only)

