#pragma once

#include <map>
#include <string>

class DriveSpeedRamping {
public:
    void reset();
    std::map<std::string, double> apply(const std::map<std::string, double>& targets, double max_delta);

private:
    std::map<std::string, double> current_linear_speeds_;
};
