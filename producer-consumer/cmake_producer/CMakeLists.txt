cmake_minimum_required(VERSION 3.14)
project(inter_docker_communication)

set(AMQP_ROOT ${CMAKE_SOURCE_DIR}/AMQP-CPP)
include_directories(${AMQP_ROOT}/include)
link_directories(${AMQP_ROOT}/build/bin)

add_executable(producer producer.cpp)
target_link_libraries(producer PRIVATE amqpcpp -lev)
