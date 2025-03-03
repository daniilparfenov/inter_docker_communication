cmake_minimum_required(VERSION 3.14)
project(inter_docker_communication)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

set(AMQP_ROOT ${CMAKE_SOURCE_DIR}/AMQP-CPP)
include_directories(${AMQP_ROOT}/include)
link_directories(${AMQP_ROOT}/build/bin)

add_executable(producer producer.cpp)
target_link_libraries(producer PRIVATE amqpcpp ${OpenCV_LIBS} -lev)

# Указываем путь к директориям с входными/выходными изображениями
set(INPUT_DIR "${CMAKE_SOURCE_DIR}/input_images")
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/output_images")

# Передаем эти пути в код через определение препроцессора
add_definitions(-DINPUT_DIR="${INPUT_DIR}")
add_definitions(-DOUTPUT_DIR="${OUTPUT_DIR}")
