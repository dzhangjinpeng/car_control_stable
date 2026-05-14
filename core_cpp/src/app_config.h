#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ControlConfig {
    double loop_period_s = 0.002;
    double deadzone = 0.05;
    double max_linear_speed_mps = 0.15;
    double max_steering_degrees = 12.0;
    double mode2_speed_scale = 0.25;
    double mode2_max_steering_degrees = 8.0;
    double mode0_rotation_wheel_speed_rad_s = 1.0;
    double mode0_fixed_steer_degrees = 62.9;
    double mode0_position_tolerance_rad = 0.01745;
    double mode1_turn_speed_min_scale = 0.6;
    double wheel_radius_m = 0.0855;
    double wheelbase_m = 0.62;
    double track_width_m = 0.486;
    double gear_ratio = 3.0;
    double steering_motor_speed_limit_rad_s = 1.5;
};

struct HardwareConfig {
    std::string serial_number = "14AA044B241402B10DDBDAFE448040BB";
    std::uint32_t nom_baud = 1000000;
    std::uint32_t dat_baud = 5000000;
    std::vector<int> drive_motor_ids = {1, 2, 3, 4};
    std::vector<int> steer_motor_ids = {5, 6, 7, 8};
    std::vector<int> inverted_drive_motor_ids = {2, 3};
};

struct SafetyConfig {
    double max_drive_motor_speed_rad_s = 3.0;
    double max_steer_motor_position_rad = 1.2;
    bool require_mock_for_now = true;
};

struct AppConfig {
    ControlConfig control;
    HardwareConfig hardware;
    SafetyConfig safety;
};

ControlConfig load_control_config(const std::string& path);
HardwareConfig load_hardware_config(const std::string& path);
SafetyConfig load_safety_config(const std::string& path);

bool contains_int(const std::vector<int>& values, int value);
