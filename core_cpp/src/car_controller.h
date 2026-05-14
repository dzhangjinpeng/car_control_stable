#pragma once

#include "app_config.h"
#include "motor_client.h"
#include "types.h"

class CarController {
public:
    CarController(ControlConfig control, HardwareConfig hardware, ControlMode initial_mode);

    MotorCommand update(const DriverInput& input, const MotorClient& motors);
    void send(const MotorCommand& command, MotorClient* motors) const;
    MotorCommand read_feedback(const MotorCommand& command, const MotorClient& motors) const;
    void stop_drive(MotorClient* motors) const;
    ControlMode mode() const;

private:
    MotorCommand update_mode0(const DriverInput& input, const MotorClient& motors);
    MotorCommand update_mode1(const DriverInput& input, double speed_scale, double max_steer_deg) const;
    void apply_mode_button(const DriverInput& input);
    void reset_mode_state();

    ControlConfig control_;
    HardwareConfig hardware_;
    ControlMode mode_;
    bool last_mode_button_ = false;
    bool rotation_ready_ = false;
};
