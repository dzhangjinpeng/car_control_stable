#include "input_source.h"

#ifdef __linux__
#include "gamepad.h"
#endif

#include <stdexcept>
#include <utility>

DriverInput NeutralInputSource::poll(std::uint64_t) {
    return DriverInput{};
}

std::string NeutralInputSource::name() const {
    return "neutral";
}

DemoInputSource::DemoInputSource(std::uint64_t max_loops) : max_loops_(max_loops == 0 ? 1 : max_loops) {}

DriverInput DemoInputSource::poll(std::uint64_t loop_index) {
    DriverInput input;
    const std::uint64_t segment = max_loops_ / 4 == 0 ? 1 : max_loops_ / 4;
    if (loop_index < segment) {
        return input;
    }
    if (loop_index < segment * 2) {
        input.left_y = -0.5;
        return input;
    }
    if (loop_index < segment * 3) {
        input.left_y = -0.4;
        input.left_x = 0.35;
        return input;
    }
    input.emergency_stop_button = true;
    return input;
}

std::string DemoInputSource::name() const {
    return "demo";
}

GamepadInputSource::GamepadInputSource(InputConfig config) : config_(std::move(config)) {
#ifdef __linux__
    auto* gamepad = new Gamepad(config_.device_path);
    if (!gamepad->open()) {
        delete gamepad;
        throw std::runtime_error("failed to open gamepad: " + config_.device_path);
    }
    impl_ = gamepad;
#else
    throw std::runtime_error("gamepad input is only supported on Linux board builds");
#endif
}

GamepadInputSource::~GamepadInputSource() {
#ifdef __linux__
    delete static_cast<Gamepad*>(impl_);
#endif
}

DriverInput GamepadInputSource::poll(std::uint64_t) {
    DriverInput input;
#ifdef __linux__
    auto* gamepad = static_cast<Gamepad*>(impl_);
    gamepad->update();
    input.left_x = gamepad->getAxis(config_.left_x_axis);
    input.left_y = gamepad->getAxis(config_.left_y_axis);
    input.right_x = gamepad->getAxis(config_.right_x_axis);
    input.right_y = gamepad->getAxis(config_.right_y_axis);
    input.mode_button = gamepad->getButton(config_.mode_button) != 0;
    input.steering_lock_button = gamepad->getButton(config_.steering_lock_button) != 0;
    input.drive_direction_button = gamepad->getButton(config_.drive_direction_button) != 0;
    input.emergency_stop_button = gamepad->getButton(config_.emergency_stop_button) != 0;
#endif
    return input;
}

std::string GamepadInputSource::name() const {
    return "gamepad";
}
