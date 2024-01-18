cmake_minimum_required(VERSION 3.24)
project(PaintTheMost_Game)

set(CMAKE_CXX_STANDARD 17)

include_directories(src/client)
include_directories(src/server)
add_executable(PaintTheMost_Server src/server/server.cpp)
add_executable(PaintTheMost_Client src/client/client.cpp)
include_directories(/usr/local/include)

find_package(SFML 2.5 COMPONENTS system window graphics network audio REQUIRED)
include_directories(${SFML_INCLUDE_DIRS})
target_link_libraries(PaintTheMost_Server sfml-system sfml-window sfml-graphics sfml-audio sfml-network)
target_link_libraries(PaintTheMost_Client sfml-system sfml-window sfml-graphics sfml-audio sfml-network)