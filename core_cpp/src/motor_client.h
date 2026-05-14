#pragma once

#include <map>
#include <string>
#include <vector>

class MotorClient {
public:
    virtual ~MotorClient() = default;

    virtual void open() = 0;
    virtual void close() = 0;
    virtual void enable_all() = 0;
    virtual void disable_all() = 0;
    virtual void control_velocity(int motor_id, double velocity_rad_s) = 0;
    virtual void control_position_velocity(int motor_id, double position_rad, double velocity_rad_s) = 0;
    virtual double get_position(int motor_id) const = 0;
    virtual double get_velocity(int motor_id) const = 0;
    virtual double get_torque(int motor_id) const = 0;
    virtual double get_target_position(int motor_id) const = 0;
    virtual double get_target_velocity(int motor_id) const = 0;
    virtual void set_zero_position(int motor_id) = 0;
    virtual void save_motor_param(int motor_id) = 0;
    virtual std::vector<std::string> drain_events() = 0;
};
