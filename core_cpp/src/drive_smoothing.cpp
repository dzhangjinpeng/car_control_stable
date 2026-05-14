#include "drive_smoothing.h"

#include <algorithm>
#include <cmath>

void DriveSpeedRamping::reset() {
    current_linear_speeds_.clear();
}

std::map<std::string, double> DriveSpeedRamping::apply(
    const std::map<std::string, double>& targets,
    double max_delta) {
    max_delta = std::max(0.0, max_delta);
    std::map<std::string, double> shaped;
    for (const auto& item : targets) {
        double current = 0.0;
        const auto existing = current_linear_speeds_.find(item.first);
        if (existing != current_linear_speeds_.end()) {
            current = existing->second;
        }

        const double delta = item.second - current;
        if (delta > max_delta) {
            current += max_delta;
        } else if (delta < -max_delta) {
            current -= max_delta;
        } else {
            current = item.second;
        }
        current_linear_speeds_[item.first] = current;
        shaped[item.first] = current;
    }
    return shaped;
}
