#pragma once

#include "app_config.h"
#include "motor_client.h"
#include "types.h"

class CarController {
public:
    CarController(ControlConfig control, HardwareConfig hardware);

    MotorCommand update(const DriverInput& input) const;
    void send(const MotorCommand& command, MotorClient* motors) const;
    MotorCommand read_feedback(const MotorCommand& command, const MotorClient& motors) const;
    void stop_drive(MotorClient* motors) const;

private:
    ControlConfig control_;
    HardwareConfig hardware_;
};

