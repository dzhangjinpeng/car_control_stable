#pragma once

#include "types.h"

#include <cstdint>
#include <string>

class InputSource {
public:
    virtual ~InputSource() = default;
    virtual DriverInput poll(std::uint64_t loop_index) = 0;
    virtual std::string name() const = 0;
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

