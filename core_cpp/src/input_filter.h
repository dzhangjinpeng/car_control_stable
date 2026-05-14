#pragma once

#include "app_config.h"
#include "types.h"

class InputSmoother {
public:
    explicit InputSmoother(ControlConfig config);

    void reset();
    void reset(const DriverInput& input);
    DriverInput apply(const DriverInput& input);

private:
    ControlConfig config_;
    double left_x_ = 0.0;
    double left_y_ = 0.0;
    double right_x_ = 0.0;
    double right_y_ = 0.0;
    bool initialized_ = false;
};
