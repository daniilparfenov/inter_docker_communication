cmake_minimum_required(VERSION 3.14)
project(inter_docker_communication)

# OpenCV include
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

include_directories(include)

add_library(image_inverting src/image_inverting.cpp)
target_link_libraries(image_inverting PUBLIC ${OpenCV_LIBS})

add_executable(client src/client.cpp)
target_link_libraries(client PRIVATE image_inverting)

add_executable(server src/server.cpp)
target_link_libraries(server PRIVATE image_inverting)