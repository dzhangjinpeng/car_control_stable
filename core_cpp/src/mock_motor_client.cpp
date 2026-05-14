#include "mock_motor_client.h"

#include <sstream>

namespace {

double map_value_or_zero(const std::map<int, double>& values, int key) {
    const auto it = values.find(key);
    return it == values.end() ? 0.0 : it->second;
}

std::string command_event(const std::string& name, int motor_id, double value) {
    std::ostringstream out;
    out << name << ":" << motor_id << ":" << value;
    return out.str();
}

}  // namespace

void MockMotorClient::open() {
    opened_ = true;
    events_.push_back("open");
}

void MockMotorClient::close() {
    events_.push_back("close");
    opened_ = false;
}

void MockMotorClient::enable_all() {
    enabled_ = true;
    events_.push_back("enable_all");
}

void MockMotorClient::disable_all() {
    enabled_ = false;
    events_.push_back("disable_all");
}

void MockMotorClient::control_velocity(int motor_id, double velocity_rad_s) {
    target_velocities_[motor_id] = velocity_rad_s;
    velocities_[motor_id] = enabled_ ? velocity_rad_s : 0.0;
    events_.push_back(command_event("velocity", motor_id, velocity_rad_s));
}

void MockMotorClient::control_position_velocity(int motor_id, double position_rad, double velocity_rad_s) {
    target_positions_[motor_id] = position_rad;
    target_velocities_[motor_id] = velocity_rad_s;
    positions_[motor_id] = enabled_ ? position_rad : positions_[motor_id];
    velocities_[motor_id] = enabled_ ? velocity_rad_s : 0.0;
    events_.push_back(command_event("position", motor_id, position_rad));
}

double MockMotorClient::get_position(int motor_id) const {
    return map_value_or_zero(positions_, motor_id);
}

double MockMotorClient::get_velocity(int motor_id) const {
    return map_value_or_zero(velocities_, motor_id);
}

double MockMotorClient::get_target_position(int motor_id) const {
    return map_value_or_zero(target_positions_, motor_id);
}

double MockMotorClient::get_target_velocity(int motor_id) const {
    return map_value_or_zero(target_velocities_, motor_id);
}

std::vector<std::string> MockMotorClient::drain_events() {
    auto events = events_;
    events_.clear();
    return events;
}

