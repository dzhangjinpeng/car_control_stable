#include "app_config.h"

#include <fstream>
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

}  // namespace

ControlConfig load_control_config(const std::string& path) {
    ControlConfig config;
    const std::string text = read_text_if_exists(path);
    if (text.empty()) {
        return config;
    }
    config.loop_period_s = number_value(text, "loop_period_s", config.loop_period_s);
    config.deadzone = number_value(text, "deadzone", config.deadzone);
    config.max_linear_speed_mps = number_value(text, "max_linear_speed_mps", config.max_linear_speed_mps);
    config.max_steering_degrees = number_value(text, "max_steering_degrees", config.max_steering_degrees);
    config.wheel_radius_m = number_value(text, "wheel_radius_m", config.wheel_radius_m);
    config.gear_ratio = number_value(text, "gear_ratio", config.gear_ratio);
    config.steering_motor_speed_limit_rad_s = number_value(
        text,
        "steering_motor_speed_limit_rad_s",
        config.steering_motor_speed_limit_rad_s);

    if (config.loop_period_s <= 0.0) {
        throw std::runtime_error("control.loop_period_s must be positive");
    }
    if (config.wheel_radius_m <= 0.0) {
        throw std::runtime_error("control.wheel_radius_m must be positive");
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
    config.drive_motor_ids = int_array_value(text, "drive_motor_ids", config.drive_motor_ids);
    config.steer_motor_ids = int_array_value(text, "steer_motor_ids", config.steer_motor_ids);
    config.inverted_drive_motor_ids = int_array_value(
        text,
        "inverted_drive_motor_ids",
        config.inverted_drive_motor_ids);
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

bool contains_int(const std::vector<int>& values, int value) {
    for (const int item : values) {
        if (item == value) {
            return true;
        }
    }
    return false;
}

