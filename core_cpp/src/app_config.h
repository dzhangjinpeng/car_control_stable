#pragma once

#include <map>
#include <cstdint>
#include <string>
#include <vector>

struct ControlConfig {
    double loop_period_s = 0.002;
    double telemetry_interval_s = 0.1;
    double deadzone = 0.05;
    double max_linear_speed_mps = 0.15;
    double max_steering_degrees = 12.0;
    double throttle_smoothing_alpha = 0.18;
    double steering_smoothing_alpha = 0.22;
    double throttle_curve_power = 1.2;
    double steering_curve_power = 1.4;
    double drive_speed_ramp_mps_per_s = 3.0;
    double mode2_speed_scale = 0.25;
    double mode2_max_steering_degrees = 8.0;
    double mode2_turn_speed_min_scale = 0.75;
    double mode2_turn_speed_curve_power = 1.0;
    double mode0_rotation_wheel_speed_rad_s = 1.0;
    double mode0_fixed_steer_degrees = 62.9;
    double mode0_position_tolerance_rad = 0.01745;
    double mode1_turn_speed_min_scale = 0.6;
    double mode1_turn_speed_curve_power = 1.0;
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
    std::vector<int> motor_ids = {1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<int> drive_motor_ids = {1, 2, 3, 4};
    std::vector<int> steer_motor_ids = {5, 6, 7, 8};
    std::vector<int> inverted_drive_motor_ids = {2, 3};
    std::map<std::string, int> drive_motor_roles = {
        {"front_left", 3},
        {"front_right", 4},
        {"rear_left", 2},
        {"rear_right", 1},
    };
    std::map<std::string, int> steer_motor_roles = {
        {"front_left", 6},
        {"front_right", 7},
        {"rear_left", 5},
        {"rear_right", 8},
    };
};

struct SafetyConfig {
    double max_drive_motor_speed_rad_s = 3.0;
    double max_steer_motor_position_rad = 1.2;
    bool require_mock_for_now = true;
};

struct InputConfig {
    std::string device_path = "/dev/input/js0";
    int left_x_axis = 0;
    int left_y_axis = 1;
    int right_x_axis = 3;
    int right_y_axis = 4;
    int mode_button = 1;
    int steering_lock_button = 4;
    int drive_direction_button = 5;
    int emergency_stop_button = 6;
};

struct NetworkConfig {
    std::string bind_host = "0.0.0.0";
    int port = 23333;
    double timeout_s = 0.2;
    double poll_timeout_s = 0.0;
};

struct AppConfig {
    ControlConfig control;
    HardwareConfig hardware;
    SafetyConfig safety;
    InputConfig input;
    NetworkConfig network;
};

ControlConfig load_control_config(const std::string& path);
ControlConfig load_control_config(const std::string& path, const std::string& profile_name, const std::string& profiles_path);
HardwareConfig load_hardware_config(const std::string& path);
SafetyConfig load_safety_config(const std::string& path);
InputConfig load_input_config(const std::string& path);
NetworkConfig load_network_config(const std::string& path);

bool contains_int(const std::vector<int>& values, int value);
