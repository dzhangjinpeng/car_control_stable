cmake_minimum_required(VERSION 3.16)
project(car_control_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CAR_CONTROL_WITH_DAMIAO "Build Damiao USB-CANFD hardware client" OFF)

add_executable(car_control_core
    src/app_config.cpp
    src/car_controller.cpp
    src/damiao_motor_client.cpp
    src/drive_smoothing.cpp
    src/input_filter.cpp
    src/input_source.cpp
    src/main.cpp
    src/mock_motor_client.cpp
    src/safety_manager.cpp
    src/telemetry.cpp
)

target_include_directories(car_control_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/damiao_core/include
)

if (UNIX)
    target_sources(car_control_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/damiao_core/src/gamepad.cpp
    )
endif()

if (WIN32)
    target_link_libraries(car_control_core PRIVATE ws2_32)
endif()

if (CAR_CONTROL_WITH_DAMIAO)
    find_package(Threads REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    target_compile_definitions(car_control_core PRIVATE CAR_CONTROL_WITH_DAMIAO=1)
    target_include_directories(car_control_core PRIVATE ${LIBUSB_INCLUDE_DIRS})
    target_sources(car_control_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/damiao_core/src/protocol/damiao.cpp
    )
    target_link_libraries(car_control_core PRIVATE
        Threads::Threads
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/damiao_core/lib/libu2canfd.a
        ${LIBUSB_LIBRARIES}
    )
endif()
