#pragma once

#include "motor_client.h"

#include <map>
#include <string>
#include <vector>

class MockMotorClient final : public MotorClient {
public:
    void open() override;
    void close() override;
    void enable_all() override;
    void disable_all() override;
    void control_velocity(int motor_id, double velocity_rad_s) override;
    void control_position_velocity(int motor_id, double position_rad, double velocity_rad_s) override;
    double get_position(int motor_id) const override;
    double get_velocity(int motor_id) const override;
    double get_torque(int motor_id) const override;
    double get_target_position(int motor_id) const override;
    double get_target_velocity(int motor_id) const override;
    void set_zero_position(int motor_id) override;
    void save_motor_param(int motor_id) override;
    std::vector<std::string> drain_events() override;

private:
    bool opened_ = false;
    bool enabled_ = false;
    std::map<int, double> positions_;
    std::map<int, double> velocities_;
    std::map<int, double> torques_;
    std::map<int, double> target_positions_;
    std::map<int, double> target_velocities_;
    std::vector<std::string> events_;
};
