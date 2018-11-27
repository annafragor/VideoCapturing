cmake_minimum_required(VERSION 2.8.7)
project(test)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(main ${SOURCE_FILES} main.cpp)
add_executable(create_mask ${SOURCE_FILES} create_mask.cpp)

target_link_libraries(main ${OpenCV_LIBS})
target_link_libraries(create_mask ${OpenCV_LIBS})
