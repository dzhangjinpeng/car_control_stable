#include "app_config.h"

#include <fstream>
#include <cstdint>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace {

std::string read_text_if_exists(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return "";
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

double number_value(const std::string& text, const std::string& key, double fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        return std::stod(match[1].str());
    }
    return fallback;
}

std::string string_value(const std::string& text, const std::string& key, const std::string& fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        return match[1].str();
    }
    return fallback;
}

bool bool_value(const std::string& text, const std::string& key, bool fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        return match[1].str() == "true";
    }
    return fallback;
}

std::vector<int> int_array_value(const std::string& text, const std::string& key, std::vector<int> fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\\[([^\\]]*)\\]");
    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        return fallback;
    }
    std::vector<int> values;
    const std::string body = match[1].str();
    const std::regex number_pattern("-?[0-9]+");
    auto begin = std::sregex_iterator(body.begin(), body.end(), number_pattern);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        values.push_back(std::stoi(it->str()));
    }
    return values.empty() ? fallback : values;
}

std::map<std::string, int> string_int_map_value(
    const std::string& text,
    const std::string& key,
    std::map<std::string, int> fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\\{([^\\}]*)\\}");
    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        return fallback;
    }
    std::map<std::string, int> values;
    const std::string body = match[1].str();
    const std::regex item_pattern("\"([^\"]+)\"\\s*:\\s*(-?[0-9]+)");
    auto begin = std::sregex_iterator(body.begin(), body.end(), item_pattern);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        values[(*it)[1].str()] = std::stoi((*it)[2].str());
    }
    return values.empty() ? fallback : values;
}

void apply_control_profile(ControlConfig* config, const std::string& profiles_text, const std::string& profile_name) {
    if (profile_name.empty()) {
        return;
    }
    const std::regex section_pattern("\"" + profile_name + "\"\\s*:\\s*\\{([^\\}]*)\\}");
    std::smatch match;
    if (!std::regex_search(profiles_text, match, section_pattern)) {
        throw std::runtime_error("unknown control profile: " + profile_name);
    }
    const std::string profile = match[1].str();
    config->max_linear_speed_mps = number_value(profile, "max_linear_speed_mps", config->max_linear_speed_mps);
    config->mode2_speed_scale = number_value(profile, "mode2_speed_scale", config->mode2_speed_scale);
    config->mode1_turn_speed_min_scale = number_value(
        profile,
        "mode1_turn_speed_min_scale",
        config->mode1_turn_speed_min_scale);
    config->mode2_turn_speed_min_scale = number_value(
        profile,
        "mode2_turn_speed_min_scale",
        config->mode2_turn_speed_min_scale);
    config->drive_speed_ramp_mps_per_s = number_value(
        profile,
        "drive_speed_ramp_mps_per_s",
        config->drive_speed_ramp_mps_per_s);
}

}  // namespace

ControlConfig load_control_config(const std::string& path) {
    return load_control_config(path, "", "");
}

ControlConfig load_control_config(const std::string& path, const std::string& profile_name, const std::string& profiles_path) {
    ControlConfig config;
    const std::string text = read_text_if_exists(path);
    if (!text.empty()) {
        config.loop_period_s = number_value(text, "loop_period_s", config.loop_period_s);
        config.telemetry_interval_s = number_value(text, "telemetry_interval_s", config.telemetry_interval_s);
        config.deadzone = number_value(text, "deadzone", config.deadzone);
        config.max_linear_speed_mps = number_value(text, "max_linear_speed_mps", config.max_linear_speed_mps);
        config.max_steering_degrees = number_value(text, "max_steering_degrees", config.max_steering_degrees);
        config.throttle_smoothing_alpha = number_value(
            text,
            "throttle_smoothing_alpha",
            config.throttle_smoothing_alpha);
        config.steering_smoothing_alpha = number_value(
            text,
            "steering_smoothing_alpha",
            config.steering_smoothing_alpha);
        config.throttle_curve_power = number_value(text, "throttle_curve_power", config.throttle_curve_power);
        config.steering_curve_power = number_value(text, "steering_curve_power", config.steering_curve_power);
        config.drive_speed_ramp_mps_per_s = number_value(
            text,
            "drive_speed_ramp_mps_per_s",
            config.drive_speed_ramp_mps_per_s);
        config.mode2_speed_scale = number_value(text, "mode2_speed_scale", config.mode2_speed_scale);
        config.mode2_max_steering_degrees = number_value(
            text,
            "mode2_max_steering_degrees",
            config.mode2_max_steering_degrees);
        config.mode2_turn_speed_min_scale = number_value(
            text,
            "mode2_turn_speed_min_scale",
            config.mode2_turn_speed_min_scale);
        config.mode2_turn_speed_curve_power = number_value(
            text,
            "mode2_turn_speed_curve_power",
            config.mode2_turn_speed_curve_power);
        config.mode0_rotation_wheel_speed_rad_s = number_value(
            text,
            "mode0_rotation_wheel_speed_rad_s",
            config.mode0_rotation_wheel_speed_rad_s);
        config.mode0_fixed_steer_degrees = number_value(
            text,
            "mode0_fixed_steer_degrees",
            config.mode0_fixed_steer_degrees);
        config.mode0_position_tolerance_rad = number_value(
            text,
            "mode0_position_tolerance_rad",
            config.mode0_position_tolerance_rad);
        config.mode1_turn_speed_min_scale = number_value(
            text,
            "mode1_turn_speed_min_scale",
            config.mode1_turn_speed_min_scale);
        config.mode1_turn_speed_curve_power = number_value(
            text,
            "mode1_turn_speed_curve_power",
            config.mode1_turn_speed_curve_power);
        config.wheel_radius_m = number_value(text, "wheel_radius_m", config.wheel_radius_m);
        config.wheelbase_m = number_value(text, "wheelbase_m", config.wheelbase_m);
        config.track_width_m = number_value(text, "track_width_m", config.track_width_m);
        config.gear_ratio = number_value(text, "gear_ratio", config.gear_ratio);
        config.steering_motor_speed_limit_rad_s = number_value(
            text,
            "steering_motor_speed_limit_rad_s",
            config.steering_motor_speed_limit_rad_s);
    }
    if (!profile_name.empty()) {
        const std::string profiles_text = read_text_if_exists(profiles_path.empty() ? "configs/control_profiles.json" : profiles_path);
        if (profiles_text.empty()) {
            throw std::runtime_error("failed to read control profiles: " + profiles_path);
        }
        apply_control_profile(&config, profiles_text, profile_name);
    }

    if (config.loop_period_s <= 0.0) {
        throw std::runtime_error("control.loop_period_s must be positive");
    }
    if (config.telemetry_interval_s <= 0.0) {
        throw std::runtime_error("control.telemetry_interval_s must be positive");
    }
    if (config.wheel_radius_m <= 0.0) {
        throw std::runtime_error("control.wheel_radius_m must be positive");
    }
    if (config.wheelbase_m <= 0.0 || config.track_width_m <= 0.0) {
        throw std::runtime_error("control wheelbase_m and track_width_m must be positive");
    }
    if (config.gear_ratio <= 0.0) {
        throw std::runtime_error("control.gear_ratio must be positive");
    }
    return config;
}

HardwareConfig load_hardware_config(const std::string& path) {
    HardwareConfig config;
    const std::string text = read_text_if_exists(path);
    if (text.empty()) {
        return config;
    }
    config.serial_number = string_value(text, "serial_number", config.serial_number);
    config.nom_baud = static_cast<std::uint32_t>(number_value(text, "nom_baud", config.nom_baud));
    config.dat_baud = static_cast<std::uint32_t>(number_value(text, "dat_baud", config.dat_baud));
    config.motor_ids = int_array_value(text, "motor_ids", config.motor_ids);
    config.drive_motor_ids = int_array_value(text, "drive_motor_ids", config.drive_motor_ids);
    config.steer_motor_ids = int_array_value(text, "steer_motor_ids", config.steer_motor_ids);
    config.inverted_drive_motor_ids = int_array_value(
        text,
        "inverted_drive_motor_ids",
        config.inverted_drive_motor_ids);
    config.drive_motor_roles = string_int_map_value(text, "drive_motor_roles", config.drive_motor_roles);
    config.steer_motor_roles = string_int_map_value(text, "steer_motor_roles", config.steer_motor_roles);
    return config;
}

SafetyConfig load_safety_config(const std::string& path) {
    SafetyConfig config;
    const std::string text = read_text_if_exists(path);
    if (text.empty()) {
        return config;
    }
    config.max_drive_motor_speed_rad_s = number_value(
        text,
        "max_drive_motor_speed_rad_s",
        config.max_drive_motor_speed_rad_s);
    config.max_steer_motor_position_rad = number_value(
        text,
        "max_steer_motor_position_rad",
        config.max_steer_motor_position_rad);
    config.require_mock_for_now = bool_value(text, "require_mock_for_now", config.require_mock_for_now);
    return config;
}

InputConfig load_input_config(const std::string& path) {
    InputConfig config;
    const std::string text = read_text_if_exists(path);
    if (text.empty()) {
        return config;
    }
    config.device_path = string_value(text, "device_path", config.device_path);
    config.left_x_axis = static_cast<int>(number_value(text, "left_x_axis", config.left_x_axis));
    config.left_y_axis = static_cast<int>(number_value(text, "left_y_axis", config.left_y_axis));
    config.right_x_axis = static_cast<int>(number_value(text, "right_x_axis", config.right_x_axis));
    config.right_y_axis = static_cast<int>(number_value(text, "right_y_axis", config.right_y_axis));
    config.mode_button = static_cast<int>(number_value(text, "mode_button", config.mode_button));
    config.steering_lock_button = static_cast<int>(number_value(text, "steering_lock_button", config.steering_lock_button));
    config.drive_direction_button = static_cast<int>(number_value(text, "drive_direction_button", config.drive_direction_button));
    config.emergency_stop_button = static_cast<int>(number_value(text, "emergency_stop_button", config.emergency_stop_button));
    return config;
}

NetworkConfig load_network_config(const std::string& path) {
    NetworkConfig config;
    const std::string text = read_text_if_exists(path);
    if (text.empty()) {
        return config;
    }
    config.bind_host = string_value(text, "bind_host", config.bind_host);
    config.port = static_cast<int>(number_value(text, "port", config.port));
    config.timeout_s = number_value(text, "timeout_s", config.timeout_s);
    config.poll_timeout_s = number_value(text, "poll_timeout_s", config.poll_timeout_s);
    if (config.port <= 0 || config.port > 65535) {
        throw std::runtime_error("network.port must be in 1..65535");
    }
    return config;
}

bool contains_int(const std::vector<int>& values, int value) {
    for (const int item : values) {
        if (item == value) {
            return true;
        }
    }
    return false;
}
