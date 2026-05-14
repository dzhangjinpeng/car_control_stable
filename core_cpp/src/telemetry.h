#pragma once

#include "types.h"

#include <fstream>
#include <string>

class JsonlTelemetryWriter {
public:
    explicit JsonlTelemetryWriter(const std::string& path);
    void write(const LoopTelemetry& frame);
    const std::string& path() const;

private:
    std::string path_;
    std::ofstream file_;
};

std::string telemetry_summary(const LoopTelemetry& frame);

