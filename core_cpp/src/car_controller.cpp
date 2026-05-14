#include "car_controller.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr int kDriveDirectionAuto = 0;
constexpr int kDriveDirectionForwardOnly = 1;
constexpr int kDriveDirectionReverseOnly = 2;

const char* kWheelRoles[] = {"front_left", "front_right", "rear_left", "rear_right"};

double apply_deadzone(double value, double deadzone) {
    return std::abs(value) < deadzone ? 0.0 : value;
}

double degrees_to_radians(double degrees) {
    return degrees * kPi / 180.0;
}

double radians_to_degrees(double radians) {
    return radians * 180.0 / kPi;
}

double clamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

double turn_speed_scale(double steering_input, double min_scale, double curve_power) {
    const double turn = clamp01(std::abs(steering_input));
    min_scale = clamp01(min_scale);
    curve_power = std::max(1.0, curve_power);
    return min_scale + (1.0 - min_scale) * (1.0 - std::pow(turn, curve_power));
}

double apply_drive_direction(double linear_velocity, int drive_direction_mode) {
    if (drive_direction_mode == kDriveDirectionForwardOnly) {
        return std::abs(linear_velocity);
    }
    if (drive_direction_mode == kDriveDirectionReverseOnly) {
        return -std::abs(linear_velocity);
    }
    return linear_velocity;
}

int motor_id_for_role(
    const std::map<std::string, int>& role_map,
    const std::vector<int>& fallback_ids,
    const std::string& role,
    std::size_t index) {
    const auto it = role_map.find(role);
    if (it != role_map.end()) {
        return it->second;
    }
    if (index < fallback_ids.size()) {
        return fallback_ids[index];
    }
    return 0;
}

MotorSample sample(const std::string& role, int motor_id, double target, const std::string& unit) {
    return MotorSample{role, motor_id, target, 0.0, 0.0, unit};
}

std::map<std::string, double> all_wheel_speeds(double value) {
    std::map<std::string, double> speeds;
    for (const char* role : kWheelRoles) {
        speeds[role] = value;
    }
    return speeds;
}

}  // namespace

CarController::CarController(ControlConfig control, HardwareConfig hardware, ControlMode initial_mode)
    : control_(control),
      hardware_(std::move(hardware)),
      mode_(initial_mode),
      input_smoother_(control_) {}

MotorCommand CarController::update(const DriverInput& input, const MotorClient& motors) {
    apply_mode_button(input);
    apply_steering_lock_button(input);
    apply_drive_direction_button(input);

    if (input.emergency_stop_button) {
        reset_motion_state();
        last_input_ = input;
        ++loop_count_;
        return update_mode1(DriverInput{}, 0.0, 0.0, false);
    }

    const DriverInput smoothed_input = input_smoother_.apply(input);
    last_input_ = smoothed_input;

    MotorCommand command;
    if (mode_ == ControlMode::Mode0Rotation) {
        command = update_mode0(smoothed_input, motors);
    } else if (mode_ == ControlMode::Mode1Ackermann) {
        command = update_mode1(smoothed_input, 1.0, control_.max_steering_degrees, false);
    } else {
        command = update_mode1(smoothed_input, control_.mode2_speed_scale, control_.mode2_max_steering_degrees, true);
    }
    ++loop_count_;
    return command;
}

MotorCommand CarController::update_mode0(const DriverInput& input, const MotorClient& motors) {
    MotorCommand command;
    const double throttle = apply_deadzone(input.left_y, control_.deadzone);
    const double rotate = apply_deadzone(input.right_x, control_.deadzone);

    if (rotate == 0.0) {
        rotation_ready_ = false;
        double linear_velocity_mps = -throttle * control_.max_linear_speed_mps;
        linear_velocity_mps = apply_drive_direction(linear_velocity_mps, drive_direction_mode_);
        std::map<std::string, double> wheel_linear_speeds = drive_ramping_.apply(
            all_wheel_speeds(linear_velocity_mps),
            control_.drive_speed_ramp_mps_per_s * control_.loop_period_s);

        std::size_t index = 0;
        for (const char* role_cstr : kWheelRoles) {
            const std::string role(role_cstr);
            const int motor_id = motor_id_for_role(hardware_.drive_motor_roles, hardware_.drive_motor_ids, role, index);
            double target = wheel_linear_speeds[role] / control_.wheel_radius_m;
            if (contains_int(hardware_.inverted_drive_motor_ids, motor_id)) {
                target = -target;
            }
            command.drive_motors.push_back(sample(role, motor_id, target, "rad/s"));
            ++index;
        }

        index = 0;
        for (const char* role_cstr : kWheelRoles) {
            const std::string role(role_cstr);
            const int motor_id = motor_id_for_role(hardware_.steer_motor_roles, hardware_.steer_motor_ids, role, index);
            command.steer_motors.push_back(sample(role, motor_id, 0.0, "rad"));
            ++index;
        }
        return command;
    }

    std::map<std::string, double> steer_targets;
    steer_targets["front_left"] = -control_.mode0_fixed_steer_degrees;
    steer_targets["front_right"] = control_.mode0_fixed_steer_degrees;
    steer_targets["rear_left"] = -control_.mode0_fixed_steer_degrees;
    steer_targets["rear_right"] = control_.mode0_fixed_steer_degrees;

    std::size_t index = 0;
    for (const char* role_cstr : kWheelRoles) {
        const std::string role(role_cstr);
        const int motor_id = motor_id_for_role(hardware_.steer_motor_roles, hardware_.steer_motor_ids, role, index);
        const double target = degrees_to_radians(steer_targets[role]) * control_.gear_ratio;
        command.steer_motors.push_back(sample(role, motor_id, target, "rad"));
        ++index;
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
    std::map<std::string, double> wheel_linear_speeds;
    wheel_linear_speeds["front_left"] = speed * direction * control_.wheel_radius_m;
    wheel_linear_speeds["front_right"] = -speed * direction * control_.wheel_radius_m;
    wheel_linear_speeds["rear_left"] = -speed * direction * control_.wheel_radius_m;
    wheel_linear_speeds["rear_right"] = speed * direction * control_.wheel_radius_m;
    wheel_linear_speeds = drive_ramping_.apply(
        wheel_linear_speeds,
        control_.drive_speed_ramp_mps_per_s * control_.loop_period_s);

    index = 0;
    for (const char* role_cstr : kWheelRoles) {
        const std::string role(role_cstr);
        const int motor_id = motor_id_for_role(hardware_.drive_motor_roles, hardware_.drive_motor_ids, role, index);
        double target = wheel_linear_speeds[role] / control_.wheel_radius_m;
        if (contains_int(hardware_.inverted_drive_motor_ids, motor_id)) {
            target = -target;
        }
        command.drive_motors.push_back(sample(role, motor_id, target, "rad/s"));
        ++index;
    }
    return command;
}

MotorCommand CarController::update_mode1(
    const DriverInput& input,
    double speed_scale,
    double max_steer_deg,
    bool mode2) {
    MotorCommand command;

    const double throttle = apply_deadzone(input.left_y, control_.deadzone);
    double steering = apply_deadzone(input.left_x, control_.deadzone);
    if (!mode2 && steering_locked_) {
        steering = 0.0;
    }

    double linear_velocity_mps = -throttle * control_.max_linear_speed_mps * speed_scale;
    linear_velocity_mps = apply_drive_direction(linear_velocity_mps, drive_direction_mode_);

    const double min_scale = mode2 ? control_.mode2_turn_speed_min_scale : control_.mode1_turn_speed_min_scale;
    const double curve_power = mode2 ? control_.mode2_turn_speed_curve_power : control_.mode1_turn_speed_curve_power;
    if (steering != 0.0) {
        linear_velocity_mps *= turn_speed_scale(steering, min_scale, curve_power);
    }

    std::map<std::string, double> steering_degrees;
    std::map<std::string, double> wheel_linear_speeds = all_wheel_speeds(linear_velocity_mps);
    for (const char* role : kWheelRoles) {
        steering_degrees[role] = 0.0;
    }

    const double inner_rad = degrees_to_radians(std::abs(steering) * max_steer_deg);
    if (std::abs(steering) > 1e-6 && inner_rad > 1e-6) {
        const double sign = steering > 0.0 ? 1.0 : -1.0;
        const double half_track = control_.track_width_m / 2.0;
        const double center_radius = control_.wheelbase_m / std::tan(inner_rad) + half_track;
        const double outer_deg = radians_to_degrees(std::atan(control_.wheelbase_m / (center_radius + half_track)));
        const double inner_deg = std::abs(steering) * max_steer_deg;

        if (sign > 0.0) {
            steering_degrees["front_left"] = outer_deg;
            steering_degrees["front_right"] = inner_deg;
        } else {
            steering_degrees["front_left"] = -inner_deg;
            steering_degrees["front_right"] = -outer_deg;
        }

        if (std::abs(linear_velocity_mps) > 1e-9) {
            const double inner_side_radius = center_radius - half_track;
            const double outer_side_radius = center_radius + half_track;
            const double front_inner_radius = std::hypot(inner_side_radius, control_.wheelbase_m);
            const double front_outer_radius = std::hypot(outer_side_radius, control_.wheelbase_m);

            std::map<std::string, double> radii;
            if (sign > 0.0) {
                radii["front_left"] = front_outer_radius;
                radii["front_right"] = front_inner_radius;
                radii["rear_left"] = outer_side_radius;
                radii["rear_right"] = inner_side_radius;
            } else {
                radii["front_left"] = front_inner_radius;
                radii["front_right"] = front_outer_radius;
                radii["rear_left"] = inner_side_radius;
                radii["rear_right"] = outer_side_radius;
            }
            for (const char* role : kWheelRoles) {
                wheel_linear_speeds[role] = linear_velocity_mps * (radii[role] / center_radius);
            }
        }
    }

    wheel_linear_speeds = drive_ramping_.apply(
        wheel_linear_speeds,
        control_.drive_speed_ramp_mps_per_s * control_.loop_period_s);

    std::size_t index = 0;
    for (const char* role_cstr : kWheelRoles) {
        const std::string role(role_cstr);
        const int motor_id = motor_id_for_role(hardware_.drive_motor_roles, hardware_.drive_motor_ids, role, index);
        double target = wheel_linear_speeds[role] / control_.wheel_radius_m;
        if (contains_int(hardware_.inverted_drive_motor_ids, motor_id)) {
            target = -target;
        }
        command.drive_motors.push_back(sample(role, motor_id, target, "rad/s"));
        ++index;
    }

    index = 0;
    for (const char* role_cstr : kWheelRoles) {
        const std::string role(role_cstr);
        const int motor_id = motor_id_for_role(hardware_.steer_motor_roles, hardware_.steer_motor_ids, role, index);
        const double target = degrees_to_radians(steering_degrees[role]) * control_.gear_ratio;
        command.steer_motors.push_back(sample(role, motor_id, target, "rad"));
        ++index;
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

bool CarController::steering_locked() const {
    return steering_locked_;
}

std::string CarController::drive_direction_name() const {
    if (drive_direction_mode_ == kDriveDirectionForwardOnly) {
        return "forward_only";
    }
    if (drive_direction_mode_ == kDriveDirectionReverseOnly) {
        return "reverse_only";
    }
    return "auto";
}

const DriverInput& CarController::last_input() const {
    return last_input_;
}

void CarController::apply_mode_button(const DriverInput& input) {
    if (input.mode_button && !last_mode_button_) {
        const int next = (static_cast<int>(mode_) + 1) % 3;
        mode_ = static_cast<ControlMode>(next);
        if (mode_ == ControlMode::Mode1Ackermann) {
            steering_locked_ = true;
        }
        reset_motion_state();
    }
    last_mode_button_ = input.mode_button;
}

void CarController::apply_steering_lock_button(const DriverInput& input) {
    if (input.steering_lock_button && !last_steering_lock_button_) {
        steering_locked_ = !steering_locked_;
        reset_motion_state();
    }
    last_steering_lock_button_ = input.steering_lock_button;
}

void CarController::apply_drive_direction_button(const DriverInput& input) {
    if (input.drive_direction_button && !last_drive_direction_button_) {
        drive_direction_mode_ = (drive_direction_mode_ + 1) % 3;
        reset_motion_state();
    }
    last_drive_direction_button_ = input.drive_direction_button;
}

void CarController::reset_mode_state() {
    rotation_ready_ = false;
}

void CarController::reset_motion_state() {
    reset_mode_state();
    input_smoother_.reset();
    drive_ramping_.reset();
}
