#include "app_config.h"
#include "car_controller.h"
#include "damiao_motor_client.h"
#include "input_source.h"
#include "mock_motor_client.h"
#include "safety_manager.h"
#include "telemetry.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <csignal>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <thread>
#endif

namespace {

std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running = false;
}

struct CliOptions {
    bool mock = false;
    bool quiet = false;
    std::uint64_t max_loops = 1000;
    std::string input = "neutral";
    ControlMode initial_mode = ControlMode::Mode2SafeDebug;
    std::string gamepad_device = "/dev/input/js0";
    std::string control_config = "configs/control.json";
    std::string hardware_config = "configs/hardware.json";
    std::string safety_config = "configs/safety.json";
    std::string telemetry_file = "telemetry.jsonl";
};

void print_usage() {
    std::cout
        << "Usage: car_control_core --mock [options]\n"
        << "\n"
        << "Options:\n"
        << "  --mock                         Use MockMotorClient. Required in phase 1.\n"
        << "  --input neutral|demo|gamepad   Input source. Default: neutral.\n"
        << "  --mode mode0|mode1|mode2       Initial control mode. Default: mode2.\n"
        << "  --gamepad-device PATH          Linux joystick device. Default: /dev/input/js0.\n"
        << "  --max-loops N                  Number of control loops. Default: 1000.\n"
        << "  --control-config PATH          Control config JSON path.\n"
        << "  --hardware-config PATH         Hardware config JSON path.\n"
        << "  --safety-config PATH           Safety config JSON path.\n"
        << "  --telemetry-file PATH          JSONL telemetry output. Default: telemetry.jsonl.\n"
        << "  --quiet                        Only print final summary.\n";
}

CliOptions parse_args(int argc, char** argv) {
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const auto require_value = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("missing value for " + name);
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            print_usage();
            std::exit(0);
        } else if (arg == "--mock") {
            options.mock = true;
        } else if (arg == "--quiet") {
            options.quiet = true;
        } else if (arg == "--input") {
            options.input = require_value(arg);
        } else if (arg == "--mode") {
            const std::string mode = require_value(arg);
            if (mode == "mode0") {
                options.initial_mode = ControlMode::Mode0Rotation;
            } else if (mode == "mode1") {
                options.initial_mode = ControlMode::Mode1Ackermann;
            } else if (mode == "mode2") {
                options.initial_mode = ControlMode::Mode2SafeDebug;
            } else {
                throw std::runtime_error("unsupported mode: " + mode);
            }
        } else if (arg == "--gamepad-device") {
            options.gamepad_device = require_value(arg);
        } else if (arg == "--max-loops") {
            options.max_loops = static_cast<std::uint64_t>(std::stoull(require_value(arg)));
        } else if (arg == "--control-config") {
            options.control_config = require_value(arg);
        } else if (arg == "--hardware-config") {
            options.hardware_config = require_value(arg);
        } else if (arg == "--safety-config") {
            options.safety_config = require_value(arg);
        } else if (arg == "--telemetry-file") {
            options.telemetry_file = require_value(arg);
        } else {
            throw std::runtime_error("unknown argument: " + arg);
        }
    }
    return options;
}

std::unique_ptr<InputSource> build_input_source(
    const std::string& name,
    std::uint64_t max_loops,
    const std::string& gamepad_device) {
    if (name == "neutral") {
        return std::unique_ptr<InputSource>(new NeutralInputSource());
    }
    if (name == "demo") {
        return std::unique_ptr<InputSource>(new DemoInputSource(max_loops));
    }
    if (name == "gamepad") {
        return std::unique_ptr<InputSource>(new GamepadInputSource(gamepad_device));
    }
    throw std::runtime_error("unsupported input source: " + name);
}

double seconds_since_epoch() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

void sleep_until_steady(std::chrono::steady_clock::time_point target) {
#ifdef _WIN32
    const auto now = std::chrono::steady_clock::now();
    if (target <= now) {
        return;
    }
    const auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(target - now).count();
    if (remaining_ms > 0) {
        Sleep(static_cast<DWORD>(remaining_ms));
    }
    while (std::chrono::steady_clock::now() < target) {
        Sleep(0);
    }
#else
    std::this_thread::sleep_until(target);
#endif
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        const CliOptions options = parse_args(argc, argv);
        AppConfig config;
        config.control = load_control_config(options.control_config);
        config.hardware = load_hardware_config(options.hardware_config);
        config.safety = load_safety_config(options.safety_config);

        if (config.safety.require_mock_for_now && !options.mock) {
            throw std::runtime_error("phase 1 only supports --mock; refusing to start hardware mode");
        }

        auto input_source = build_input_source(options.input, options.max_loops, options.gamepad_device);
        std::unique_ptr<MotorClient> motors;
        if (options.mock) {
            motors.reset(new MockMotorClient());
        } else {
            motors.reset(new DamiaoMotorClient(config.hardware));
        }
        CarController controller(config.control, config.hardware, options.initial_mode);
        SafetyManager safety(config.safety);
        JsonlTelemetryWriter telemetry(options.telemetry_file);

        const auto loop_period = std::chrono::duration<double>(config.control.loop_period_s);
        double max_loop_duration_ms = 0.0;
        double sum_loop_duration_ms = 0.0;
        std::uint64_t loops_completed = 0;

        motors->open();
        motors->enable_all();

        try {
            for (std::uint64_t loop = 0; g_running && (options.max_loops == 0 || loop < options.max_loops); ++loop) {
                const auto loop_start = std::chrono::steady_clock::now();

                SafetyState safety_state;
                DriverInput input = input_source->poll(loop);
                const DriverInput safe_input = safety.filter_input(input, &safety_state);
                MotorCommand command = controller.update(safe_input, *motors);
                command = safety.filter_command(command, &safety_state);
                controller.send(command, motors.get());
                MotorCommand feedback = controller.read_feedback(command, *motors);

                const auto loop_end = std::chrono::steady_clock::now();
                const double duration_ms = std::chrono::duration<double, std::milli>(loop_end - loop_start).count();
                max_loop_duration_ms = std::max(max_loop_duration_ms, duration_ms);
                sum_loop_duration_ms += duration_ms;
                loops_completed += 1;

                LoopTelemetry frame;
                frame.loop_index = loop;
                frame.timestamp_s = seconds_since_epoch();
                frame.loop_duration_ms = duration_ms;
                frame.mode = mode_name(controller.mode());
                frame.input_source = input_source->name();
                frame.input = input;
                frame.command = feedback;
                frame.safety = safety_state;
                telemetry.write(frame);

                if (!options.quiet && (loop == 0 || (options.max_loops > 0 && loop + 1 == options.max_loops) || loop % 100 == 0)) {
                    std::cout << telemetry_summary(frame) << "\n";
                }

                sleep_until_steady(
                    loop_start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(loop_period));
            }
        } catch (...) {
            controller.stop_drive(motors.get());
            motors->disable_all();
            motors->close();
            throw;
        }

        controller.stop_drive(motors.get());
        motors->disable_all();
        motors->close();

        const double avg_loop_duration_ms = loops_completed == 0 ? 0.0 : sum_loop_duration_ms / loops_completed;
        std::cout << "completed loops=" << loops_completed
                  << " avg_loop_duration_ms=" << avg_loop_duration_ms
                  << " max_loop_duration_ms=" << max_loop_duration_ms
                  << " telemetry=" << telemetry.path() << "\n";
        return 0;
    } catch (const std::exception& exc) {
        std::cerr << "error: " << exc.what() << "\n";
        return 1;
    }
}
