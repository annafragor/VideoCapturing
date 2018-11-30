cmake_minimum_required(VERSION 2.8.7)
project(test)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}<CircularQueue.h>)

add_library(CircularQueue STATIC ${CMAKE_CURRENT_SOURCE_DIR}/source/CircularQueue.cpp)

add_executable(queueTest ${SOURCE_FILES} applications/queueTest.cpp)

add_executable(main ${SOURCE_FILES} applications/main.cpp)
target_link_libraries(main ${OpenCV_LIBS})

add_executable(create_mask ${SOURCE_FILES} applications/create_mask.cpp)
target_link_libraries(create_mask ${OpenCV_LIBS})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)
