cmake_minimum_required(VERSION 3.16)
project(car_control_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(car_control_core
    src/app_config.cpp
    src/car_controller.cpp
    src/input_source.cpp
    src/main.cpp
    src/mock_motor_client.cpp
    src/safety_manager.cpp
    src/telemetry.cpp
)

target_include_directories(car_control_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
