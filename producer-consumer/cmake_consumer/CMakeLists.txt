cmake_minimum_required(VERSION 3.14)
project(inter_docker_communication)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

set(AMQP_ROOT ${CMAKE_SOURCE_DIR}/AMQP-CPP)
include_directories(${AMQP_ROOT}/include)
link_directories(${AMQP_ROOT}/build/bin)

add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE amqpcpp ${OpenCV_LIBS} -lev)
