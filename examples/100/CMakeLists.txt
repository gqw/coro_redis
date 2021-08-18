cmake_minimum_required (VERSION 3.21)

project(test_100)

file(GLOB SOURCE_FILES	./src/*.h ./src/*.cpp)

add_definitions(-DPROJECT_NAME="${PROJECT_NAME}")
if (WIN32)
	add_definitions(-DUNICODE -D_UNICODE  )
    # add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
    add_definitions(/utf-8 /wd"4200" /wd"4996")
else()
	add_definitions(-g -O0 -Wall -std=c++20)
endif()

find_package(spdlog CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(Libevent CONFIG REQUIRED)

add_executable(${PROJECT_NAME}  ${SOURCE_FILES} )
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
target_link_libraries(${PROJECT_NAME} PRIVATE hiredis::hiredis libevent::core
	libevent::extra spdlog::spdlog spdlog::spdlog_header_only)
