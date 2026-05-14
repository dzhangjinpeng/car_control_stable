#include "input_filter.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

double clamp_axis(double value) {
    return std::max(-1.0, std::min(1.0, value));
}

double shape_axis(double value, double power) {
    value = clamp_axis(value);
    if (value == 0.0) {
        return 0.0;
    }
    const double sign = value > 0.0 ? 1.0 : -1.0;
    return sign * std::pow(std::abs(value), std::max(1.0, power));
}

double smooth_axis(double previous, double target, double alpha) {
    alpha = std::max(0.0, std::min(1.0, alpha));
    return previous + (target - previous) * alpha;
}

}  // namespace

InputSmoother::InputSmoother(ControlConfig config) : config_(std::move(config)) {}

void InputSmoother::reset() {
    left_x_ = 0.0;
    left_y_ = 0.0;
    right_x_ = 0.0;
    right_y_ = 0.0;
    initialized_ = true;
}

void InputSmoother::reset(const DriverInput& input) {
    left_x_ = shape_axis(input.left_x, config_.steering_curve_power);
    left_y_ = shape_axis(input.left_y, config_.throttle_curve_power);
    right_x_ = shape_axis(input.right_x, config_.steering_curve_power);
    right_y_ = shape_axis(input.right_y, config_.throttle_curve_power);
    initialized_ = true;
}

DriverInput InputSmoother::apply(const DriverInput& input) {
    if (!initialized_) {
        reset();
    }

    const double target_left_x = shape_axis(input.left_x, config_.steering_curve_power);
    const double target_right_x = shape_axis(input.right_x, config_.steering_curve_power);
    const double target_left_y = shape_axis(input.left_y, config_.throttle_curve_power);
    const double target_right_y = shape_axis(input.right_y, config_.throttle_curve_power);

    left_x_ = smooth_axis(left_x_, target_left_x, config_.steering_smoothing_alpha);
    right_x_ = smooth_axis(right_x_, target_right_x, config_.steering_smoothing_alpha);
    left_y_ = smooth_axis(left_y_, target_left_y, config_.throttle_smoothing_alpha);
    right_y_ = smooth_axis(right_y_, target_right_y, config_.throttle_smoothing_alpha);

    DriverInput shaped = input;
    shaped.left_x = left_x_;
    shaped.left_y = left_y_;
    shaped.right_x = right_x_;
    shaped.right_y = right_y_;
    return shaped;
}
