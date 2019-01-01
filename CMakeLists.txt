cmake_minimum_required(VERSION 3.9)
project(3PC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.openmpi")

find_package(MPI REQUIRED)
find_package(Threads REQUIRED)
include_directories(SYSTEM ${MPI_CXX_INCLUDE_PATH})

file(GLOB SOURCE_FILES "src/communication/*" "src/logging/*" "src/util/*" "src/processes/*")
include_directories(src)

add_executable(3PC src/Main.cpp ${SOURCE_FILES})
target_link_libraries(3PC ${MPI_CXX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
