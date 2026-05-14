#include "car_controller.h"

#include <cmath>
#include <utility>

namespace {

constexpr double kPi = 3.14159265358979323846;

double apply_deadzone(double value, double deadzone) {
    return std::abs(value) < deadzone ? 0.0 : value;
}

double degrees_to_radians(double degrees) {
    return degrees * kPi / 180.0;
}

std::string role_for_index(int index, bool steer) {
    static const char* roles[] = {"front_left", "front_right", "rear_left", "rear_right"};
    if (index >= 0 && index < 4) {
        return roles[index];
    }
    return steer ? "steer" : "drive";
}

}  // namespace

CarController::CarController(ControlConfig control, HardwareConfig hardware)
    : control_(control), hardware_(std::move(hardware)) {}

MotorCommand CarController::update(const DriverInput& input) const {
    MotorCommand command;

    const double throttle = apply_deadzone(input.left_y, control_.deadzone);
    const double steering = apply_deadzone(input.left_x, control_.deadzone);
    const double linear_velocity_mps = -throttle * control_.max_linear_speed_mps;
    const double wheel_speed_rad_s = linear_velocity_mps / control_.wheel_radius_m;

    for (std::size_t i = 0; i < hardware_.drive_motor_ids.size(); ++i) {
        const int motor_id = hardware_.drive_motor_ids[i];
        double target = wheel_speed_rad_s;
        if (contains_int(hardware_.inverted_drive_motor_ids, motor_id)) {
            target = -target;
        }
        command.drive_motors.push_back(MotorSample{
            role_for_index(static_cast<int>(i), false),
            motor_id,
            target,
            0.0,
            0.0,
            "rad/s"});
    }

    const double steer_output_deg = steering * control_.max_steering_degrees;
    const double steer_motor_axis_rad = degrees_to_radians(steer_output_deg) * control_.gear_ratio;
    for (std::size_t i = 0; i < hardware_.steer_motor_ids.size(); ++i) {
        const int motor_id = hardware_.steer_motor_ids[i];
        command.steer_motors.push_back(MotorSample{
            role_for_index(static_cast<int>(i), true),
            motor_id,
            steer_motor_axis_rad,
            0.0,
            0.0,
            "rad"});
    }

    return command;
}

void CarController::send(const MotorCommand& command, MotorClient* motors) const {
    for (const auto& motor : command.drive_motors) {
        motors->control_velocity(motor.motor_id, motor.target);
    }
    for (const auto& motor : command.steer_motors) {
        motors->control_position_velocity(
            motor.motor_id,
            motor.target,
            control_.steering_motor_speed_limit_rad_s);
    }
}

MotorCommand CarController::read_feedback(const MotorCommand& command, const MotorClient& motors) const {
    MotorCommand feedback = command;
    for (auto& motor : feedback.drive_motors) {
        motor.actual = motors.get_velocity(motor.motor_id);
        motor.error = motor.target - motor.actual;
    }
    for (auto& motor : feedback.steer_motors) {
        motor.actual = motors.get_position(motor.motor_id);
        motor.error = motor.target - motor.actual;
    }
    return feedback;
}

void CarController::stop_drive(MotorClient* motors) const {
    for (const int motor_id : hardware_.drive_motor_ids) {
        motors->control_velocity(motor_id, 0.0);
    }
}
