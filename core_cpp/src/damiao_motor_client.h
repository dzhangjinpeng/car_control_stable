#pragma once

#include "app_config.h"
#include "motor_client.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#ifdef CAR_CONTROL_WITH_DAMIAO
#include "protocol/damiao.h"
#endif

class DamiaoMotorClient final : public MotorClient {
public:
    explicit DamiaoMotorClient(HardwareConfig config);

    void open() override;
    void close() override;
    void enable_all() override;
    void disable_all() override;
    void control_velocity(int motor_id, double velocity_rad_s) override;
    void control_position_velocity(int motor_id, double position_rad, double velocity_rad_s) override;
    double get_position(int motor_id) const override;
    double get_velocity(int motor_id) const override;
    double get_torque(int motor_id) const override;
    double get_target_position(int motor_id) const override;
    double get_target_velocity(int motor_id) const override;
    void set_zero_position(int motor_id) override;
    void save_motor_param(int motor_id) override;
    std::vector<std::string> drain_events() override;

private:
    HardwareConfig config_;
    std::map<int, double> target_positions_;
    std::map<int, double> target_velocities_;
    std::vector<std::string> events_;
#ifdef CAR_CONTROL_WITH_DAMIAO
    std::vector<damiao::DmActData> init_data_;
    std::shared_ptr<damiao::Motor_Control> control_;
#endif
};
