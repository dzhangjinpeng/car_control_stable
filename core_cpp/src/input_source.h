#pragma once

#include "app_config.h"
#include "types.h"

#include <cstdint>
#include <memory>
#include <string>

class InputSource {
public:
    virtual ~InputSource() = default;
    virtual DriverInput poll(std::uint64_t loop_index) = 0;
    virtual std::string name() const = 0;
    virtual std::string link_state() const { return name(); }
    virtual int remote_seq() const { return -1; }
    virtual double remote_latency_s() const { return -1.0; }
    virtual bool remote_stale() const { return false; }
};

class NeutralInputSource final : public InputSource {
public:
    DriverInput poll(std::uint64_t loop_index) override;
    std::string name() const override;
};

class DemoInputSource final : public InputSource {
public:
    explicit DemoInputSource(std::uint64_t max_loops);
    DriverInput poll(std::uint64_t loop_index) override;
    std::string name() const override;

private:
    std::uint64_t max_loops_;
};

class GamepadInputSource final : public InputSource {
public:
    explicit GamepadInputSource(InputConfig config);
    ~GamepadInputSource() override;

    DriverInput poll(std::uint64_t loop_index) override;
    std::string name() const override;

private:
    InputConfig config_;
    void* impl_ = nullptr;
};

class UdpRemoteInputSource final : public InputSource {
public:
    explicit UdpRemoteInputSource(NetworkConfig config);
    ~UdpRemoteInputSource() override;

    DriverInput poll(std::uint64_t loop_index) override;
    std::string name() const override;
    std::string link_state() const override;
    int remote_seq() const override;
    double remote_latency_s() const override;
    bool remote_stale() const override;

private:
    NetworkConfig config_;
    void* socket_state_ = nullptr;
    DriverInput latest_input_;
    int latest_seq_ = -1;
    double latest_received_s_ = -1.0;
    bool latest_active_ = false;
    std::string link_state_ = "remote_idle";
};

class HybridInputSource final : public InputSource {
public:
    HybridInputSource(std::unique_ptr<InputSource> local, std::unique_ptr<UdpRemoteInputSource> remote);
    DriverInput poll(std::uint64_t loop_index) override;
    std::string name() const override;
    std::string link_state() const override;
    int remote_seq() const override;
    double remote_latency_s() const override;
    bool remote_stale() const override;

private:
    std::unique_ptr<InputSource> local_;
    std::unique_ptr<UdpRemoteInputSource> remote_;
    std::string source_name_ = "hybrid_local";
    std::string link_state_ = "local";
};
