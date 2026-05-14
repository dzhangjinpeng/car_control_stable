#include "car_controller.h"

#include <algorithm>
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

MotorSample sample(const std::string& role, int motor_id, double target, const std::string& unit) {
    return MotorSample{role, motor_id, target, 0.0, 0.0, unit};
}

}  // namespace

CarController::CarController(ControlConfig control, HardwareConfig hardware, ControlMode initial_mode)
    : control_(control), hardware_(std::move(hardware)), mode_(initial_mode) {}

MotorCommand CarController::update(const DriverInput& input, const MotorClient& motors) {
    apply_mode_button(input);
    if (input.emergency_stop_button) {
        reset_mode_state();
        return update_mode1(DriverInput{}, 0.0, 0.0);
    }
    if (mode_ == ControlMode::Mode0Rotation) {
        return update_mode0(input, motors);
    }
    if (mode_ == ControlMode::Mode1Ackermann) {
        return update_mode1(input, 1.0, control_.max_steering_degrees);
    }
    return update_mode1(input, control_.mode2_speed_scale, control_.mode2_max_steering_degrees);
}

MotorCommand CarController::update_mode0(const DriverInput& input, const MotorClient& motors) {
    MotorCommand command;
    const double throttle = apply_deadzone(input.left_y, control_.deadzone);
    const double rotate = apply_deadzone(input.right_x, control_.deadzone);

    if (rotate == 0.0) {
        rotation_ready_ = false;
        const double linear_velocity_mps = -throttle * control_.max_linear_speed_mps;
        const double wheel_speed_rad_s = linear_velocity_mps / control_.wheel_radius_m;
        for (std::size_t i = 0; i < hardware_.drive_motor_ids.size(); ++i) {
            const int motor_id = hardware_.drive_motor_ids[i];
            double target = wheel_speed_rad_s;
            if (contains_int(hardware_.inverted_drive_motor_ids, motor_id)) {
                target = -target;
            }
            command.drive_motors.push_back(sample(role_for_index(static_cast<int>(i), false), motor_id, target, "rad/s"));
        }
        for (std::size_t i = 0; i < hardware_.steer_motor_ids.size(); ++i) {
            command.steer_motors.push_back(
                sample(role_for_index(static_cast<int>(i), true), hardware_.steer_motor_ids[i], 0.0, "rad"));
        }
        return command;
    }

    for (std::size_t i = 0; i < hardware_.steer_motor_ids.size(); ++i) {
        const int motor_id = hardware_.steer_motor_ids[i];
        double sign = 1.0;
        if (i == 0 || i == 2) {
            sign = -1.0;
        }
        const double target = degrees_to_radians(sign * control_.mode0_fixed_steer_degrees) * control_.gear_ratio;
        command.steer_motors.push_back(sample(role_for_index(static_cast<int>(i), true), motor_id, target, "rad"));
    }

    if (!rotation_ready_) {
        bool all_reached = true;
        for (const auto& motor : command.steer_motors) {
            if (std::abs(motors.get_position(motor.motor_id) - motor.target) > control_.mode0_position_tolerance_rad) {
                all_reached = false;
                break;
            }
        }
        rotation_ready_ = all_reached;
    }

    const double direction = rotate > 0.0 ? 1.0 : -1.0;
    const double speed = rotation_ready_ ? std::abs(rotate) * control_.mode0_rotation_wheel_speed_rad_s : 0.0;
    const double pattern[] = {direction, -direction, -direction, direction};
    for (std::size_t i = 0; i < hardware_.drive_motor_ids.size(); ++i) {
        const int motor_id = hardware_.drive_motor_ids[i];
        double target = speed * pattern[std::min<std::size_t>(i, 3)];
        if (contains_int(hardware_.inverted_drive_motor_ids, motor_id)) {
            target = -target;
        }
        command.drive_motors.push_back(sample(role_for_index(static_cast<int>(i), false), motor_id, target, "rad/s"));
    }
    return command;
}

MotorCommand CarController::update_mode1(const DriverInput& input, double speed_scale, double max_steer_deg) const {
    MotorCommand command;

    const double throttle = apply_deadzone(input.left_y, control_.deadzone);
    const double steering = apply_deadzone(input.left_x, control_.deadzone);
    double linear_velocity_mps = -throttle * control_.max_linear_speed_mps * speed_scale;

    if (steering != 0.0 && control_.mode1_turn_speed_min_scale < 1.0) {
        const double turn_scale = 1.0 - (1.0 - control_.mode1_turn_speed_min_scale) * std::min(1.0, std::abs(steering));
        linear_velocity_mps *= turn_scale;
    }

    const double max_inner_rad = degrees_to_radians(max_steer_deg);
    const double inner_rad = steering * max_inner_rad;
    double steer_fl = inner_rad;
    double steer_fr = inner_rad;
    double front_left_speed = linear_velocity_mps;
    double front_right_speed = linear_velocity_mps;
    double rear_left_speed = linear_velocity_mps;
    double rear_right_speed = linear_velocity_mps;

    if (std::abs(inner_rad) > 1e-6 && std::abs(linear_velocity_mps) > 1e-9) {
        const double abs_inner = std::abs(inner_rad);
        const double radius_inner = control_.wheelbase_m / std::tan(abs_inner);
        const double radius_center = radius_inner + control_.track_width_m / 2.0;
        const double radius_outer = radius_center + control_.track_width_m / 2.0;
        const double outer_rad = std::atan(control_.wheelbase_m / radius_outer);
        const double sign = inner_rad > 0.0 ? 1.0 : -1.0;
        steer_fl = sign > 0.0 ? sign * abs_inner : sign * outer_rad;
        steer_fr = sign > 0.0 ? sign * outer_rad : sign * abs_inner;
        const double left_scale = sign > 0.0 ? radius_inner / radius_center : radius_outer / radius_center;
        const double right_scale = sign > 0.0 ? radius_outer / radius_center : radius_inner / radius_center;
        front_left_speed = linear_velocity_mps * left_scale;
        rear_left_speed = linear_velocity_mps * left_scale;
        front_right_speed = linear_velocity_mps * right_scale;
        rear_right_speed = linear_velocity_mps * right_scale;
    }

    const double speeds_mps[] = {front_left_speed, front_right_speed, rear_left_speed, rear_right_speed};
    for (std::size_t i = 0; i < hardware_.drive_motor_ids.size(); ++i) {
        const int motor_id = hardware_.drive_motor_ids[i];
        double target = speeds_mps[std::min<std::size_t>(i, 3)] / control_.wheel_radius_m;
        if (contains_int(hardware_.inverted_drive_motor_ids, motor_id)) {
            target = -target;
        }
        command.drive_motors.push_back(sample(role_for_index(static_cast<int>(i), false), motor_id, target, "rad/s"));
    }

    const double steer_targets[] = {
        steer_fl * control_.gear_ratio,
        steer_fr * control_.gear_ratio,
        0.0,
        0.0,
    };
    for (std::size_t i = 0; i < hardware_.steer_motor_ids.size(); ++i) {
        command.steer_motors.push_back(
            sample(role_for_index(static_cast<int>(i), true), hardware_.steer_motor_ids[i], steer_targets[std::min<std::size_t>(i, 3)], "rad"));
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

ControlMode CarController::mode() const {
    return mode_;
}

void CarController::apply_mode_button(const DriverInput& input) {
    if (input.mode_button && !last_mode_button_) {
        const int next = (static_cast<int>(mode_) + 1) % 3;
        mode_ = static_cast<ControlMode>(next);
        reset_mode_state();
    }
    last_mode_button_ = input.mode_button;
}

void CarController::reset_mode_state() {
    rotation_ready_ = false;
}

