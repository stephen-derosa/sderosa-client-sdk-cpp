/*
 * Copyright 2026 LiveKit
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Servo control example -- connects to a LiveKit room and exposes
 * Feetech STS servo motors via data tracks.
 *
 * For each servo (pan=0, tilt=1):
 *   - Publishes current state on "servo-{index}.state"
 *   - Receives angular velocity commands on "servo-{index}.input_vel"
 *
 * A 30 Hz control loop applies the most recent velocity command to each
 * motor, zeroing speed if no command has arrived within TIMEOUT (300 ms).
 *
 * Usage:
 *   ServoControl <serial-port> <ws-url> <token> [--controller <identity>]
 *   ServoControl /dev/ttyUSB0 --controller operator
 *       (with LIVEKIT_URL / LIVEKIT_TOKEN env vars)
 *   ServoControl --sim --controller operator
 *       (simulated servos, no hardware required)
 *
 * The token must grant publish + subscribe permissions.
 *
 * --controller <identity>
 *         Identity of the remote participant that sends velocity commands
 *         (i.e. the ServoController's LiveKit token identity). Required
 *         to receive commands.
 *
 * --sim   Run without hardware. Servo position is modelled by integrating
 *         the commanded velocity over time (instant velocity achievement).
 *         The serial port argument is not required in this mode.
 */

#include "livekit_bridge/livekit_bridge.h"
#include "lk_log.h"

#include "SCServo.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;
using Clock = std::chrono::steady_clock;

// ---------------------------------------------------------------------------
// Servo index enum
// ---------------------------------------------------------------------------
enum ServoIndex : uint8_t {
  SERVO_PAN = 0,
  SERVO_TILT = 1,
  SERVO_COUNT = 2,
};

static const char *kServoNames[SERVO_COUNT] = {"pan", "tilt"};

// Motor IDs on the serial bus (1-indexed as required by the protocol).
// Servo index 0 (pan)  -> motor ID 1
// Servo index 1 (tilt) -> motor ID 2
static constexpr uint8_t kMotorIds[SERVO_COUNT] = {1, 2};

static constexpr int kBaudRate = 1000000;
static constexpr int kControlRateHz = 30;
static constexpr auto kControlPeriod =
    std::chrono::microseconds(1000000 / kControlRateHz);
static constexpr auto kVelocityTimeout = std::chrono::milliseconds(300);
static constexpr uint8_t kAcceleration = 50;

// Position step → degrees: 4096 steps = 360° → 1 step ≈ 0.088°
static constexpr double kStepsPerDegree = 4096.0 / 360.0;
// Speed steps/s → degrees/s: same ratio
static constexpr double kSpeedStepsPerDegPerSec = 4096.0 / 360.0;

// ---------------------------------------------------------------------------
// Per-servo velocity command (written from data-track callback, read by
// control loop). Protected by a mutex to also guard the timestamp.
// ---------------------------------------------------------------------------
struct VelocityCommand {
  std::mutex mu;
  double angular_velocity_deg_s{0.0};
  Clock::time_point received_at{};
  bool ever_received{false};
};

// ---------------------------------------------------------------------------
// Simulated servo -- models instant velocity achievement and integrates
// position over time.  Used when --sim is passed (no hardware required).
// ---------------------------------------------------------------------------
struct SimServo {
  double position_steps{2048.0};
  s16 speed_steps_s{0};
  Clock::time_point last_update{Clock::now()};

  static constexpr int kMaxSteps = 4096;
  static constexpr int kSimVoltage = 120;    // 12.0 V
  static constexpr int kSimTemperature = 25; // °C

  void update(s16 speed, Clock::time_point now) {
    double dt = std::chrono::duration<double>(now - last_update).count();
    position_steps += speed_steps_s * dt;

    // Wrap into [0, kMaxSteps)
    position_steps = std::fmod(position_steps, static_cast<double>(kMaxSteps));
    if (position_steps < 0.0)
      position_steps += kMaxSteps;

    speed_steps_s = speed;
    last_update = now;
  }

  void readState(int &pos, int &spd, int &load, int &volt, int &temp) const {
    pos = static_cast<int>(position_steps) % kMaxSteps;
    spd = speed_steps_s;
    load = 0;
    volt = kSimVoltage;
    temp = kSimTemperature;
  }
};

static std::atomic<bool> g_running{true};
static void handleSignal(int) { g_running.store(false); }

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string makeTrackName(int index, const char *suffix) {
  return "servo-" + std::to_string(index) + "." + suffix;
}

static json buildStateJson(int index, int position, int speed, int load,
                           int voltage, int temperature) {
  return json{
      {"servo", kServoNames[index]},
      {"index", index},
      {"position_steps", position},
      {"position_deg", position / kStepsPerDegree},
      {"speed_steps_s", speed},
      {"speed_deg_s", speed / kSpeedStepsPerDegPerSec},
      {"load", load},
      {"voltage_dv", voltage},
      {"temperature_c", temperature},
  };
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  // ---- Parse arguments ----
  std::string serial_port;
  std::string url;
  std::string token;
  std::string remote_identity;
  bool sim_mode = false;

  auto is_ws_url = [](const std::string &s) {
    return (s.size() >= 5 && s.compare(0, 5, "ws://") == 0) ||
           (s.size() >= 6 && s.compare(0, 6, "wss://") == 0);
  };

  std::vector<std::string> positional;
  for (int i = 1; i < argc; ++i) {
    if ((std::strcmp(argv[i], "--identity") == 0 ||
         std::strcmp(argv[i], "--controller") == 0) &&
        i + 1 < argc) {
      remote_identity = argv[++i];
    } else if (std::strcmp(argv[i], "--sim") == 0) {
      sim_mode = true;
    } else {
      positional.push_back(argv[i]);
    }
  }

  // First positional that looks like /dev/* is the serial port;
  // first ws(s):// is the URL; remainder is the token.
  for (const auto &arg : positional) {
    if (serial_port.empty() && arg.compare(0, 5, "/dev/") == 0) {
      serial_port = arg;
    } else if (url.empty() && is_ws_url(arg)) {
      url = arg;
    } else if (token.empty()) {
      token = arg;
    }
  }

  if (url.empty()) {
    const char *e = std::getenv("LIVEKIT_URL");
    if (e)
      url = e;
  }
  if (token.empty()) {
    const char *e = std::getenv("LIVEKIT_TOKEN");
    if (e)
      token = e;
  }

  if ((!sim_mode && serial_port.empty()) || url.empty() || token.empty()) {
    LK_LOG_ERROR(
        "Usage: ServoControl [--sim] <serial-port> [<ws-url> <token>] "
        "[--controller <identity>]\n"
        "       Environment: LIVEKIT_URL, LIVEKIT_TOKEN\n"
        "       --sim         Run with simulated servos (no serial port "
        "required)\n"
        "       --controller  Identity of the remote participant sending "
        "velocity commands\n"
        "                     (must match the controller's LiveKit token "
        "identity)");
    return 1;
  }

  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);

  // ---- Initialize servo bus (or sim) ----
  SMS_STS servo;
  SimServo sim_servos[SERVO_COUNT];

  if (sim_mode) {
    LK_LOG_INFO("[servo_control] Running in SIMULATION mode");
  } else {
    if (!servo.begin(kBaudRate, serial_port.c_str())) {
      LK_LOG_ERROR("[servo_control] Failed to open serial port {}",
                   serial_port);
      return 1;
    }
    LK_LOG_INFO("[servo_control] Opened serial port {} @ {} baud", serial_port,
                kBaudRate);

    for (int i = 0; i < SERVO_COUNT; ++i) {
      if (!servo.InitMotor(kMotorIds[i], SMS_STS_MODE_WHEEL_CLOSED, 1)) {
        LK_LOG_ERROR("[servo_control] Failed to init motor {} (servo {})",
                     kMotorIds[i], kServoNames[i]);
        servo.end();
        return 1;
      }
      LK_LOG_INFO("[servo_control] Initialized {} servo (motor ID {})",
                  kServoNames[i], kMotorIds[i]);
    }
  }

  // ---- Connect to LiveKit ----
  livekit_bridge::LiveKitBridge bridge;
  livekit::RoomOptions options;
  options.auto_subscribe = true;

  LK_LOG_INFO("[servo_control] Connecting to {} ...", url);
  if (!bridge.connect(url, token, options)) {
    LK_LOG_ERROR("[servo_control] Failed to connect to LiveKit");
    if (!sim_mode)
      servo.end();
    return 1;
  }
  LK_LOG_INFO("[servo_control] Connected to LiveKit room");

  // ---- Create outgoing state tracks ----
  std::shared_ptr<livekit_bridge::BridgeDataTrack> state_tracks[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; ++i) {
    const auto track_name = makeTrackName(i, "state");
    LK_LOG_INFO("[servo_control] Creating state track {}", track_name);
    state_tracks[i] = bridge.createDataTrack(track_name);
    LK_LOG_INFO("[servo_control] Publishing state track {}",
                state_tracks[i]->name());
  }

  // ---- Set up velocity command reception ----
  VelocityCommand vel_cmds[SERVO_COUNT];

  auto registerInputCallback = [&](int index) {
    const auto track_name = makeTrackName(index, "input_vel");
    LK_LOG_INFO("[servo_control] Creating input velocity track {}", track_name);
    auto cb = [index, track_name, remote_identity,
               &vel_cmds](const std::vector<std::uint8_t> &payload,
                          std::optional<std::uint64_t> /*user_timestamp*/) {
      try {
        auto j = json::parse(payload.begin(), payload.end());
        double vel = j.value("angular_velocity_deg_s", 0.0);
        {
          std::lock_guard<std::mutex> lock(vel_cmds[index].mu);
          vel_cmds[index].angular_velocity_deg_s = vel;
          vel_cmds[index].received_at = Clock::now();
          vel_cmds[index].ever_received = true;
        }
        LK_LOG_DEBUG("[servo_control] {} vel command: {:.1f} deg/s",
                     kServoNames[index], vel);
      } catch (const json::exception &e) {
        LK_LOG_WARN("[servo_control] Bad JSON on {}: {}", kServoNames[index],
                    e.what());
      }
    };

    if (!remote_identity.empty()) {
      bridge.setOnDataFrameCallback(remote_identity, track_name, cb);
      LK_LOG_INFO(
          "[servo_control] Listening for {} from controller identity \"{}\"",
          track_name, remote_identity);
    } else {
      LK_LOG_WARN("[servo_control] No --controller specified; {} will not "
                  "receive commands",
                  track_name);
    }
  };

  for (int i = 0; i < SERVO_COUNT; ++i) {
    registerInputCallback(i);
  }

  // ---- 30 Hz control + state publish loop ----
  LK_LOG_INFO("[servo_control] Starting {} Hz control loop", kControlRateHz);

  // the last time we've logged
  Clock::time_point last_log_time = Clock::now();

  auto next_tick = Clock::now();
  while (g_running.load()) {
    next_tick += kControlPeriod;
    auto now = Clock::now();
    const auto should_log =
        now - last_log_time > std::chrono::milliseconds(250);

    for (int i = 0; i < SERVO_COUNT; ++i) {
      // Determine target speed
      s16 speed_steps = 0;
      {
        std::lock_guard<std::mutex> lock(vel_cmds[i].mu);
        if (vel_cmds[i].ever_received &&
            (now - vel_cmds[i].received_at) < kVelocityTimeout) {
          speed_steps = static_cast<s16>(vel_cmds[i].angular_velocity_deg_s *
                                         kSpeedStepsPerDegPerSec);
        }
      }

      int pos, spd, load, volt, temp;

      if (sim_mode) {
        sim_servos[i].update(speed_steps, now);
        sim_servos[i].readState(pos, spd, load, volt, temp);
      } else {
        servo.WriteSpe(kMotorIds[i], speed_steps, kAcceleration);

        if (servo.FeedBack(kMotorIds[i])) {
          pos = servo.ReadPos(-1);
          spd = servo.ReadSpeed(-1);
          load = servo.ReadLoad(-1);
          volt = servo.ReadVoltage(-1);
          temp = servo.ReadTemper(-1);
        } else {
          LK_LOG_WARN("[servo_control] FeedBack failed for {} (motor {})",
                      kServoNames[i], kMotorIds[i]);
          continue;
        }
      }

      if (should_log) {
        {
          last_log_time = now;
          auto state = buildStateJson(i, pos, spd, load, volt, temp);
          LK_LOG_INFO("[servo_control] {} state: {}", kServoNames[i],
                      state.dump());
          std::string msg = state.dump();
          std::vector<std::uint8_t> payload(msg.begin(), msg.end());
          state_tracks[i]->pushFrame(payload);
        }
      }
    }

    std::this_thread::sleep_until(next_tick);
  }

  // ---- Cleanup ----
  LK_LOG_INFO("[servo_control] Shutting down...");

  if (!sim_mode) {
    for (int i = 0; i < SERVO_COUNT; ++i) {
      servo.WriteSpe(kMotorIds[i], 0, kAcceleration);
      servo.EnableTorque(kMotorIds[i], 0);
    }
  }

  for (int i = 0; i < SERVO_COUNT; ++i) {
    if (!remote_identity.empty()) {
      const auto track_name = makeTrackName(i, "input_vel");
      bridge.clearOnDataFrameCallback(remote_identity, track_name);
      LK_LOG_INFO("[servo_control] Unsubscribing from {}/{}", remote_identity,
                  track_name);
    }
    state_tracks[i].reset();
  }

  bridge.disconnect();
  if (!sim_mode)
    servo.end();

  LK_LOG_INFO("[servo_control] Done.");
  return 0;
}
