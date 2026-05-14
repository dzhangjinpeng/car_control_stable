#include "safety_manager.h"

#include <algorithm>
#include <cmath>

SafetyManager::SafetyManager(SafetyConfig config) : config_(config) {}

DriverInput SafetyManager::filter_input(const DriverInput& input, SafetyState* state) const {
    DriverInput safe_input = input;
    if (input.emergency_stop_button) {
        safe_input.left_x = 0.0;
        safe_input.left_y = 0.0;
        safe_input.right_x = 0.0;
        safe_input.right_y = 0.0;
        if (state != nullptr) {
            state->emergency_stop = true;
            state->notice = "emergency stop active";
        }
    }
    return safe_input;
}

MotorCommand SafetyManager::filter_command(const MotorCommand& command, SafetyState* state) const {
    MotorCommand safe_command = command;
    for (auto& motor : safe_command.drive_motors) {
        const double original = motor.target;
        motor.target = std::max(
            -config_.max_drive_motor_speed_rad_s,
            std::min(config_.max_drive_motor_speed_rad_s, motor.target));
        if (state != nullptr && std::abs(original - motor.target) > 1e-9) {
            state->command_limited = true;
        }
    }
    for (auto& motor : safe_command.steer_motors) {
        const double original = motor.target;
        motor.target = std::max(
            -config_.max_steer_motor_position_rad,
            std::min(config_.max_steer_motor_position_rad, motor.target));
        if (state != nullptr && std::abs(original - motor.target) > 1e-9) {
            state->command_limited = true;
        }
    }
    if (state != nullptr && state->command_limited && state->notice.empty()) {
        state->notice = "command limited by safety manager";
    }
    return safe_command;
}

