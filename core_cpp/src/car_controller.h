#pragma once

#include "app_config.h"
#include "drive_smoothing.h"
#include "input_filter.h"
#include "motor_client.h"
#include "types.h"

#include <cstdint>
#include <string>

class CarController {
public:
    CarController(ControlConfig control, HardwareConfig hardware, ControlMode initial_mode);

    MotorCommand update(const DriverInput& input, const MotorClient& motors);
    void send(const MotorCommand& command, MotorClient* motors) const;
    MotorCommand read_feedback(const MotorCommand& command, const MotorClient& motors) const;
    void stop_drive(MotorClient* motors) const;
    ControlMode mode() const;
    bool steering_locked() const;
    std::string drive_direction_name() const;
    const DriverInput& last_input() const;

private:
    MotorCommand update_mode0(const DriverInput& input, const MotorClient& motors);
    MotorCommand update_mode1(const DriverInput& input, double speed_scale, double max_steer_deg, bool mode2);
    void apply_mode_button(const DriverInput& input);
    void apply_steering_lock_button(const DriverInput& input);
    void apply_drive_direction_button(const DriverInput& input);
    void reset_mode_state();
    void reset_motion_state();

    ControlConfig control_;
    HardwareConfig hardware_;
    ControlMode mode_;
    InputSmoother input_smoother_;
    DriveSpeedRamping drive_ramping_;
    DriverInput last_input_;
    bool last_mode_button_ = false;
    bool last_steering_lock_button_ = false;
    bool last_drive_direction_button_ = false;
    bool steering_locked_ = true;
    int drive_direction_mode_ = 0;
    bool rotation_ready_ = false;
    std::uint64_t loop_count_ = 0;
};
