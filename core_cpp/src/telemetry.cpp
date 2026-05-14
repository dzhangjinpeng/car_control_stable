#include "telemetry.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {

std::string json_bool(bool value) {
    return value ? "true" : "false";
}

void append_input_json(std::ostringstream& out, const DriverInput& input) {
    out << "\"driver_input\":{"
        << "\"left_x\":" << input.left_x << ","
        << "\"left_y\":" << input.left_y << ","
        << "\"right_x\":" << input.right_x << ","
        << "\"right_y\":" << input.right_y << ","
        << "\"mode_button\":" << json_bool(input.mode_button) << ","
        << "\"steering_lock_button\":" << json_bool(input.steering_lock_button) << ","
        << "\"drive_direction_button\":" << json_bool(input.drive_direction_button) << ","
        << "\"emergency_stop_button\":" << json_bool(input.emergency_stop_button)
        << "}";
}

void append_motor_array_json(std::ostringstream& out, const char* name, const std::vector<MotorSample>& motors) {
    out << "\"" << name << "\":[";
    for (std::size_t i = 0; i < motors.size(); ++i) {
        const auto& motor = motors[i];
        if (i != 0) {
            out << ",";
        }
        out << "{"
            << "\"role\":\"" << motor.role << "\","
            << "\"motor_id\":" << motor.motor_id << ","
            << "\"target\":" << motor.target << ","
            << "\"actual\":" << motor.actual << ","
            << "\"error\":" << motor.error << ","
            << "\"unit\":\"" << motor.unit << "\""
            << "}";
    }
    out << "]";
}

}  // namespace

JsonlTelemetryWriter::JsonlTelemetryWriter(const std::string& path) : path_(path), file_(path) {
    if (!file_) {
        throw std::runtime_error("failed to open telemetry file: " + path);
    }
}

void JsonlTelemetryWriter::write(const LoopTelemetry& frame) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6);
    out << "{"
        << "\"timestamp_s\":" << frame.timestamp_s << ","
        << "\"loop_index\":" << frame.loop_index << ","
        << "\"loop_duration_ms\":" << frame.loop_duration_ms << ","
        << "\"mode\":\"" << frame.mode << "\","
        << "\"input_source\":\"" << frame.input_source << "\",";
    append_input_json(out, frame.input);
    out << ",\"safety\":{"
        << "\"emergency_stop\":" << json_bool(frame.safety.emergency_stop) << ","
        << "\"command_limited\":" << json_bool(frame.safety.command_limited) << ","
        << "\"notice\":\"" << frame.safety.notice << "\""
        << "},";
    append_motor_array_json(out, "drive_motors", frame.command.drive_motors);
    out << ",";
    append_motor_array_json(out, "steer_motors", frame.command.steer_motors);
    out << "}\n";
    file_ << out.str();
}

const std::string& JsonlTelemetryWriter::path() const {
    return path_;
}

std::string telemetry_summary(const LoopTelemetry& frame) {
    std::ostringstream out;
    out << "loop=" << frame.loop_index
        << " duration_ms=" << std::fixed << std::setprecision(3) << frame.loop_duration_ms
        << " input=" << frame.input_source
        << " emergency=" << (frame.safety.emergency_stop ? "true" : "false");
    if (!frame.command.drive_motors.empty()) {
        out << " drive0_target=" << std::setprecision(3) << frame.command.drive_motors.front().target;
    }
    return out.str();
}

