# requires minimum cmake version
cmake_minimum_required(VERSION 3.16.0)

# produce the cmake var PROJECT_NAME
project(openpose-evaluation)

find_package(hpe-core REQUIRED)

# isaac application for testing pose + velocity fusion
add_executable(openpose-evaluation openpose-evaluation.cpp)
target_link_libraries(openpose-evaluation hpe-core::hpe-core)
install(TARGETS openpose-evaluation DESTINATION ${CMAKE_INSTALL_BINDIR})

