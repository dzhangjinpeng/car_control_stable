#pragma once

#include "app_config.h"
#include "types.h"

class SafetyManager {
public:
    explicit SafetyManager(SafetyConfig config);

    DriverInput filter_input(const DriverInput& input, SafetyState* state) const;
    MotorCommand filter_command(const MotorCommand& command, SafetyState* state) const;

private:
    SafetyConfig config_;
};

