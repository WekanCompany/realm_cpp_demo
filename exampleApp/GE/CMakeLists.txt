cmake_minimum_required(VERSION 3.15)

project (hello)

set(CMAKE_CXX_STANDARD 17)
set(REALM_CPP_NO_TESTS 1)

FIND_PACKAGE( OpenCV REQUIRED )                              
INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} )

add_subdirectory(../../ cmake-build-debug)
add_executable(hello helloworld.cpp)
target_link_libraries(hello cpprealm)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/input/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/output/)
                                        