#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class ControlMode {
    Mode0Rotation = 0,
    Mode1Ackermann = 1,
    Mode2SafeDebug = 2,
};

inline std::string mode_name(ControlMode mode) {
    switch (mode) {
        case ControlMode::Mode0Rotation:
            return "mode0_rotation";
        case ControlMode::Mode1Ackermann:
            return "mode1_ackermann";
        case ControlMode::Mode2SafeDebug:
            return "mode2_safe_debug";
    }
    return "unknown";
}

struct DriverInput {
    double left_x = 0.0;
    double left_y = 0.0;
    double right_x = 0.0;
    double right_y = 0.0;
    bool mode_button = false;
    bool steering_lock_button = false;
    bool drive_direction_button = false;
    bool emergency_stop_button = false;
};

struct MotorSample {
    std::string role;
    int motor_id = 0;
    double target = 0.0;
    double actual = 0.0;
    double error = 0.0;
    std::string unit;
};

struct MotorCommand {
    std::vector<MotorSample> drive_motors;
    std::vector<MotorSample> steer_motors;
};

struct SafetyState {
    bool emergency_stop = false;
    bool command_limited = false;
    std::string notice;
};

struct LoopTelemetry {
    std::uint64_t loop_index = 0;
    double timestamp_s = 0.0;
    double loop_duration_ms = 0.0;
    std::string mode = "mode2_safe_debug";
    std::string input_source = "neutral";
    std::string input_link_state = "local";
    int remote_seq = -1;
    double remote_latency_s = -1.0;
    bool remote_stale = false;
    DriverInput input;
    MotorCommand command;
    SafetyState safety;
};
