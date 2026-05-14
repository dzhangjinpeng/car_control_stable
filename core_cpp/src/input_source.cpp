#include "input_source.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef __linux__
#include "gamepad.h"
#endif

#include <algorithm>
#include <chrono>
#include <cstring>
#include <regex>
#include <stdexcept>
#include <utility>

namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
#endif

struct SocketState {
    SocketHandle socket = kInvalidSocket;
};

double monotonic_seconds() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

double number_field(const std::string& text, const std::string& key, double fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        return std::stod(match[1].str());
    }
    return fallback;
}

bool bool_field(const std::string& text, const std::string& key, bool fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(true|false|0|1)");
    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        return fallback;
    }
    const std::string value = match[1].str();
    return value == "true" || value == "1";
}

DriverInput parse_remote_payload(const std::string& payload, int* seq, bool* active) {
    DriverInput input;
    *seq = static_cast<int>(number_field(payload, "seq", *seq));
    *active = bool_field(payload, "active", true);
    input.left_x = std::max(-1.0, std::min(1.0, number_field(payload, "left_x", 0.0)));
    input.left_y = std::max(-1.0, std::min(1.0, number_field(payload, "left_y", 0.0)));
    input.right_x = std::max(-1.0, std::min(1.0, number_field(payload, "right_x", 0.0)));
    input.right_y = std::max(-1.0, std::min(1.0, number_field(payload, "right_y", 0.0)));
    input.mode_button = bool_field(payload, "mode_button", false);
    input.steering_lock_button = bool_field(payload, "steering_lock_button", false);
    input.drive_direction_button = bool_field(payload, "drive_direction_button", false);
    input.emergency_stop_button = bool_field(payload, "emergency_stop_button", false);
    return input;
}

void close_socket(SocketHandle socket) {
    if (socket == kInvalidSocket) {
        return;
    }
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

void make_nonblocking(SocketHandle socket) {
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
        throw std::runtime_error("failed to set UDP socket nonblocking");
    }
#else
    const int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0 || fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error("failed to set UDP socket nonblocking");
    }
#endif
}

bool would_block() {
#ifdef _WIN32
    const int err = WSAGetLastError();
    return err == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

SocketState* open_udp_socket(const NetworkConfig& config) {
#ifdef _WIN32
    static bool wsa_started = false;
    if (!wsa_started) {
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
        wsa_started = true;
    }
#endif

    auto* state = new SocketState();
    state->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (state->socket == kInvalidSocket) {
        delete state;
        throw std::runtime_error("failed to create UDP socket");
    }

    int reuse = 1;
    setsockopt(state->socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(config.port));
    if (config.bind_host.empty() || config.bind_host == "0.0.0.0") {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
#ifdef _WIN32
        const unsigned long parsed = inet_addr(config.bind_host.c_str());
        if (parsed == INADDR_NONE) {
            close_socket(state->socket);
            delete state;
            throw std::runtime_error("network.bind_host must be an IPv4 address");
        }
        addr.sin_addr.s_addr = parsed;
#else
        if (inet_pton(AF_INET, config.bind_host.c_str(), &addr.sin_addr) != 1) {
            close_socket(state->socket);
            delete state;
            throw std::runtime_error("network.bind_host must be an IPv4 address");
        }
#endif
    }

    if (bind(state->socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close_socket(state->socket);
        delete state;
        throw std::runtime_error("failed to bind UDP remote input port");
    }
    make_nonblocking(state->socket);
    return state;
}

}  // namespace

DriverInput NeutralInputSource::poll(std::uint64_t) {
    return DriverInput{};
}

std::string NeutralInputSource::name() const {
    return "neutral";
}

DemoInputSource::DemoInputSource(std::uint64_t max_loops) : max_loops_(max_loops == 0 ? 1 : max_loops) {}

DriverInput DemoInputSource::poll(std::uint64_t loop_index) {
    DriverInput input;
    const std::uint64_t segment = max_loops_ / 4 == 0 ? 1 : max_loops_ / 4;
    if (loop_index < segment) {
        return input;
    }
    if (loop_index < segment * 2) {
        input.left_y = -0.5;
        return input;
    }
    if (loop_index < segment * 3) {
        input.left_y = -0.4;
        input.left_x = 0.35;
        return input;
    }
    input.emergency_stop_button = true;
    return input;
}

std::string DemoInputSource::name() const {
    return "demo";
}

GamepadInputSource::GamepadInputSource(InputConfig config) : config_(std::move(config)) {
#ifdef __linux__
    auto* gamepad = new Gamepad(config_.device_path);
    if (!gamepad->open()) {
        delete gamepad;
        throw std::runtime_error("failed to open gamepad: " + config_.device_path);
    }
    impl_ = gamepad;
#else
    throw std::runtime_error("gamepad input is only supported on Linux board builds");
#endif
}

GamepadInputSource::~GamepadInputSource() {
#ifdef __linux__
    delete static_cast<Gamepad*>(impl_);
#endif
}

DriverInput GamepadInputSource::poll(std::uint64_t) {
    DriverInput input;
#ifdef __linux__
    auto* gamepad = static_cast<Gamepad*>(impl_);
    gamepad->update();
    input.left_x = gamepad->getAxis(config_.left_x_axis);
    input.left_y = gamepad->getAxis(config_.left_y_axis);
    input.right_x = gamepad->getAxis(config_.right_x_axis);
    input.right_y = gamepad->getAxis(config_.right_y_axis);
    input.mode_button = gamepad->getButton(config_.mode_button) != 0;
    input.steering_lock_button = gamepad->getButton(config_.steering_lock_button) != 0;
    input.drive_direction_button = gamepad->getButton(config_.drive_direction_button) != 0;
    input.emergency_stop_button = gamepad->getButton(config_.emergency_stop_button) != 0;
#endif
    return input;
}

std::string GamepadInputSource::name() const {
    return "gamepad";
}

UdpRemoteInputSource::UdpRemoteInputSource(NetworkConfig config) : config_(std::move(config)) {
    socket_state_ = open_udp_socket(config_);
}

UdpRemoteInputSource::~UdpRemoteInputSource() {
    auto* state = static_cast<SocketState*>(socket_state_);
    if (state != nullptr) {
        close_socket(state->socket);
        delete state;
        socket_state_ = nullptr;
    }
}

DriverInput UdpRemoteInputSource::poll(std::uint64_t) {
    auto* state = static_cast<SocketState*>(socket_state_);
    if (state == nullptr) {
        throw std::runtime_error("remote input socket is not open");
    }

    char buffer[4096];
    bool received = false;
    while (true) {
        sockaddr_in from;
#ifdef _WIN32
        int from_len = sizeof(from);
        const int count = recvfrom(state->socket, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
#else
        socklen_t from_len = sizeof(from);
        const int count = static_cast<int>(recvfrom(state->socket, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&from), &from_len));
#endif
        if (count <= 0) {
            if (would_block()) {
                break;
            }
            break;
        }
        buffer[count] = '\0';
        int seq = latest_seq_;
        bool active = latest_active_;
        latest_input_ = parse_remote_payload(std::string(buffer, static_cast<std::size_t>(count)), &seq, &active);
        latest_seq_ = seq;
        latest_active_ = active;
        latest_received_s_ = monotonic_seconds();
        received = true;
    }

    const double age = remote_latency_s();
    const bool stale = latest_seq_ < 0 || age < 0.0 || age > config_.timeout_s;
    if (latest_seq_ < 0) {
        link_state_ = "remote_idle";
        return DriverInput{};
    }
    if (!latest_active_) {
        link_state_ = "remote_inactive";
        return DriverInput{};
    }
    if (stale) {
        link_state_ = "remote_timeout";
        return DriverInput{};
    }
    link_state_ = received ? "remote_online" : "remote_hold";
    return latest_input_;
}

std::string UdpRemoteInputSource::name() const {
    return "remote";
}

std::string UdpRemoteInputSource::link_state() const {
    return link_state_;
}

int UdpRemoteInputSource::remote_seq() const {
    return latest_seq_;
}

double UdpRemoteInputSource::remote_latency_s() const {
    if (latest_received_s_ < 0.0) {
        return -1.0;
    }
    return monotonic_seconds() - latest_received_s_;
}

bool UdpRemoteInputSource::remote_stale() const {
    const double age = remote_latency_s();
    return latest_seq_ < 0 || age < 0.0 || age > config_.timeout_s;
}

HybridInputSource::HybridInputSource(std::unique_ptr<InputSource> local, std::unique_ptr<UdpRemoteInputSource> remote)
    : local_(std::move(local)), remote_(std::move(remote)) {}

DriverInput HybridInputSource::poll(std::uint64_t loop_index) {
    DriverInput local_input = local_->poll(loop_index);
    DriverInput remote_input = remote_->poll(loop_index);

    if (local_input.emergency_stop_button) {
        source_name_ = "hybrid_local_estop";
        link_state_ = "local_estop";
        return local_input;
    }

    const bool remote_available =
        remote_->remote_seq() >= 0 &&
        !remote_->remote_stale() &&
        remote_->link_state() != "remote_inactive";
    if (!remote_available) {
        source_name_ = "hybrid_local";
        link_state_ = remote_->link_state();
        return local_input;
    }

    remote_input.mode_button = remote_input.mode_button || local_input.mode_button;
    remote_input.steering_lock_button = remote_input.steering_lock_button || local_input.steering_lock_button;
    remote_input.drive_direction_button = remote_input.drive_direction_button || local_input.drive_direction_button;
    remote_input.emergency_stop_button = remote_input.emergency_stop_button || local_input.emergency_stop_button;
    source_name_ = "hybrid_remote";
    link_state_ = "remote_takeover";
    return remote_input;
}

std::string HybridInputSource::name() const {
    return source_name_;
}

std::string HybridInputSource::link_state() const {
    return link_state_;
}

int HybridInputSource::remote_seq() const {
    return remote_->remote_seq();
}

double HybridInputSource::remote_latency_s() const {
    return remote_->remote_latency_s();
}

bool HybridInputSource::remote_stale() const {
    return remote_->remote_stale();
}
