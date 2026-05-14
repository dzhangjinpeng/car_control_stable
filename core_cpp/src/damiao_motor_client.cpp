#include "damiao_motor_client.h"

#include <sstream>
#include <stdexcept>

namespace {

double value_or_zero(const std::map<int, double>& values, int key) {
    const auto it = values.find(key);
    return it == values.end() ? 0.0 : it->second;
}

}  // namespace

DamiaoMotorClient::DamiaoMotorClient(HardwareConfig config) : config_(std::move(config)) {}

void DamiaoMotorClient::open() {
#ifndef CAR_CONTROL_WITH_DAMIAO
    throw std::runtime_error("Damiao hardware support was not compiled; rebuild with CAR_CONTROL_WITH_DAMIAO=ON");
#else
    init_data_.clear();
    const std::uint16_t master_base = 0x10;
    for (const int motor_id : config_.drive_motor_ids) {
        init_data_.push_back(damiao::DmActData{
            damiao::DMH6215,
            damiao::VEL_MODE,
            static_cast<std::uint16_t>(motor_id),
            static_cast<std::uint16_t>(master_base + motor_id)});
    }
    for (const int motor_id : config_.steer_motor_ids) {
        init_data_.push_back(damiao::DmActData{
            damiao::DM8009,
            damiao::POS_VEL_MODE,
            static_cast<std::uint16_t>(motor_id),
            static_cast<std::uint16_t>(master_base + motor_id)});
    }
    control_ = std::make_shared<damiao::Motor_Control>(
        config_.nom_baud,
        config_.dat_baud,
        config_.serial_number,
        &init_data_);
    for (const int motor_id : config_.steer_motor_ids) {
        auto motor = control_->getMotor(static_cast<std::uint16_t>(motor_id));
        if (motor) {
            control_->switchControlMode(*motor, damiao::POS_VEL);
        }
    }
    events_.push_back("open");
#endif
}

void DamiaoMotorClient::close() {
#ifdef CAR_CONTROL_WITH_DAMIAO
    control_.reset();
#endif
    events_.push_back("close");
}

void DamiaoMotorClient::enable_all() {
#ifndef CAR_CONTROL_WITH_DAMIAO
    throw std::runtime_error("Damiao hardware support was not compiled");
#else
    if (!control_) {
        throw std::runtime_error("DamiaoMotorClient is not open");
    }
    control_->enable_all();
    events_.push_back("enable_all");
#endif
}

void DamiaoMotorClient::disable_all() {
#ifdef CAR_CONTROL_WITH_DAMIAO
    if (control_) {
        control_->disable_all();
    }
#endif
    events_.push_back("disable_all");
}

void DamiaoMotorClient::control_velocity(int motor_id, double velocity_rad_s) {
#ifndef CAR_CONTROL_WITH_DAMIAO
    (void)motor_id;
    (void)velocity_rad_s;
    throw std::runtime_error("Damiao hardware support was not compiled");
#else
    if (!control_) {
        throw std::runtime_error("DamiaoMotorClient is not open");
    }
    auto motor = control_->getMotor(static_cast<std::uint16_t>(motor_id));
    if (!motor) {
        throw std::runtime_error("drive motor not found");
    }
    control_->control_vel(*motor, static_cast<float>(velocity_rad_s));
    target_velocities_[motor_id] = velocity_rad_s;
#endif
}

void DamiaoMotorClient::control_position_velocity(int motor_id, double position_rad, double velocity_rad_s) {
#ifndef CAR_CONTROL_WITH_DAMIAO
    (void)motor_id;
    (void)position_rad;
    (void)velocity_rad_s;
    throw std::runtime_error("Damiao hardware support was not compiled");
#else
    if (!control_) {
        throw std::runtime_error("DamiaoMotorClient is not open");
    }
    auto motor = control_->getMotor(static_cast<std::uint16_t>(motor_id));
    if (!motor) {
        throw std::runtime_error("steer motor not found");
    }
    control_->control_pos_vel(*motor, static_cast<float>(position_rad), static_cast<float>(velocity_rad_s));
    target_positions_[motor_id] = position_rad;
    target_velocities_[motor_id] = velocity_rad_s;
#endif
}

double DamiaoMotorClient::get_position(int motor_id) const {
#ifdef CAR_CONTROL_WITH_DAMIAO
    if (control_) {
        auto motor = control_->getMotor(static_cast<std::uint16_t>(motor_id));
        if (motor) {
            return motor->Get_Position();
        }
    }
#else
    (void)motor_id;
#endif
    return 0.0;
}

double DamiaoMotorClient::get_velocity(int motor_id) const {
#ifdef CAR_CONTROL_WITH_DAMIAO
    if (control_) {
        auto motor = control_->getMotor(static_cast<std::uint16_t>(motor_id));
        if (motor) {
            return motor->Get_Velocity();
        }
    }
#else
    (void)motor_id;
#endif
    return 0.0;
}

double DamiaoMotorClient::get_target_position(int motor_id) const {
    return value_or_zero(target_positions_, motor_id);
}

double DamiaoMotorClient::get_target_velocity(int motor_id) const {
    return value_or_zero(target_velocities_, motor_id);
}

std::vector<std::string> DamiaoMotorClient::drain_events() {
    auto events = events_;
    events_.clear();
    return events;
}
