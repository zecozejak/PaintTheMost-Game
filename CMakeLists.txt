cmake_minimum_required(VERSION 3.24)
project(PaintTheMost_Game)

set(CMAKE_CXX_STANDARD 17)

include_directories(src/client)
include_directories(src/server)
include_directories(src/shared)
add_executable(PaintTheMost_Server src/shared/communication.cpp src/server/server.cpp src/shared/communication.h)
add_executable(PaintTheMost_Client src/shared/communication.cpp src/client/client.cpp src/shared/communication.h)
set_target_properties(PaintTheMost_Client PROPERTIES COMPILE_FLAGS "-g")
set_target_properties(PaintTheMost_Server PROPERTIES COMPILE_FLAGS "-g")
include_directories(/usr/local/include)

find_package(SFML 2.5 COMPONENTS system window graphics network audio REQUIRED)
include_directories(${SFML_INCLUDE_DIRS})
target_link_libraries(PaintTheMost_Server sfml-system sfml-window sfml-graphics sfml-audio sfml-network)
target_link_libraries(PaintTheMost_Client sfml-system sfml-window sfml-graphics sfml-audio sfml-network)