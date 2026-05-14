#include "input_source.h"

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

