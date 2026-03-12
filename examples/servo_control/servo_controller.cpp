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
 * Keyboard controller for the ServoControl example.
 *
 * Connects to the same LiveKit room and sends angular velocity commands
 * to the servo_control participant via data tracks.  Also subscribes to
 * the servo state tracks and prints a status line every 0.5 seconds.
 *
 * Controls (hold to move, release to stop):
 *   a / d  = pan  positive / negative velocity
 *   w / s  = tilt positive / negative velocity
 *   q      = quit
 *
 * Usage:
 *   ServoController <ws-url> <token> [--identity <servo-participant>]
 *   ServoController --identity robot
 *       (with LIVEKIT_URL / LIVEKIT_TOKEN env vars)
 *
 * --identity names the remote participant running ServoControl (default:
 * "robot").
 */

#include "livekit_bridge/livekit_bridge.h"
#include "lk_log.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

using json = nlohmann::json;
using Clock = std::chrono::steady_clock;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr int SERVO_COUNT = 2;
static const char *kServoNames[SERVO_COUNT] = {"pan", "tilt"};
static constexpr double kVelocityMagnitude = 60.0; // deg/s while key held

// If no keypress arrives within this window the key is considered released.
// Terminal key-repeat typically fires every ~30-50ms, so 150ms covers gaps.
static constexpr auto kKeyHoldTimeout = std::chrono::milliseconds(150);
static constexpr auto kResendInterval = std::chrono::milliseconds(100);

// ---------------------------------------------------------------------------
// Raw terminal helpers (same pattern as simple_joystick sender)
// ---------------------------------------------------------------------------

#ifndef _WIN32
static struct termios g_orig_termios;
static bool g_raw_mode_enabled = false;

static void disableRawMode() {
  if (g_raw_mode_enabled) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
    g_raw_mode_enabled = false;
  }
}

static void enableRawMode() {
  tcgetattr(STDIN_FILENO, &g_orig_termios);
  g_raw_mode_enabled = true;
  std::atexit(disableRawMode);

  struct termios raw = g_orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static int readKeyNonBlocking() {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  struct timeval tv = {0, 0};
  if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
    unsigned char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1)
      return ch;
  }
  return -1;
}
#else
static void enableRawMode() {}
static void disableRawMode() {}
static int readKeyNonBlocking() {
  if (_kbhit())
    return _getch();
  return -1;
}
#endif

// ---------------------------------------------------------------------------
// Shared state for received servo states
// ---------------------------------------------------------------------------

struct ServoState {
  std::mutex mu;
  json last_state;
  bool received{false};
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string makeTrackName(int index, const char *suffix) {
  return "servo-" + std::to_string(index) + "." + suffix;
}

static std::atomic<bool> g_running{true};
static void handleSignal(int) { g_running.store(false); }

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  std::string url;
  std::string token;
  std::string remote_identity = "servo-1";

  auto is_ws_url = [](const std::string &s) {
    return (s.size() >= 5 && s.compare(0, 5, "ws://") == 0) ||
           (s.size() >= 6 && s.compare(0, 6, "wss://") == 0);
  };

  std::vector<std::string> positional;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--identity") == 0 && i + 1 < argc) {
      remote_identity = argv[++i];
    } else {
      positional.push_back(argv[i]);
    }
  }

  for (const auto &arg : positional) {
    if (url.empty() && is_ws_url(arg)) {
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

  if (url.empty() || token.empty()) {
    LK_LOG_ERROR(
        "Usage: ServoController [<ws-url> <token>] [--identity <remote>]\n"
        "       Environment: LIVEKIT_URL, LIVEKIT_TOKEN");
    return 1;
  }

  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);

  // ---- Connect to LiveKit ----
  livekit_bridge::LiveKitBridge bridge;
  livekit::RoomOptions options;
  options.auto_subscribe = true;

  LK_LOG_INFO("[controller] Connecting to {} ...", url);
  if (!bridge.connect(url, token, options)) {
    LK_LOG_ERROR("[controller] Failed to connect to LiveKit");
    return 1;
  }
  LK_LOG_INFO("[controller] Connected. Remote servo identity: {}",
              remote_identity);

  // ---- Create velocity command tracks ----
  std::shared_ptr<livekit_bridge::BridgeDataTrack> vel_tracks[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; ++i) {
    const auto track_name = makeTrackName(i, "input_vel");
    LK_LOG_INFO("[controller] Creating input velocity track {}", track_name);
    vel_tracks[i] = bridge.createDataTrack(track_name);
    LK_LOG_INFO("[controller] Publishing {}", vel_tracks[i]->name());
  }

  // ---- Subscribe to state tracks ----
  ServoState servo_states[SERVO_COUNT];

  for (int i = 0; i < SERVO_COUNT; ++i) {
    std::string track_name = makeTrackName(i, "state");
    LK_LOG_INFO("[controller] Creating state track {}", track_name);
    bridge.setOnDataFrameCallback(
        remote_identity, track_name,
        [i, &servo_states](const std::vector<std::uint8_t> &payload,
                           std::optional<std::uint64_t>) {
          try {
            auto j = json::parse(payload.begin(), payload.end());
            std::lock_guard<std::mutex> lock(servo_states[i].mu);
            servo_states[i].last_state = std::move(j);
            servo_states[i].received = true;
          } catch (const json::exception &) {
          }
        });
    LK_LOG_INFO("[controller] Subscribed to {}/{}", remote_identity,
                track_name);
  }

  // ---- Velocity state ----
  // Desired velocity per axis while the key is held; decays to zero
  // when no keypress arrives within kKeyHoldTimeout.
  double velocities[SERVO_COUNT] = {0.0, 0.0};
  Clock::time_point last_key_time[SERVO_COUNT] = {};
  bool key_active[SERVO_COUNT] = {false, false};

  auto sendVelocity = [&](int index) {
    json j = {{"angular_velocity_deg_s", velocities[index]}};
    std::string msg = j.dump();
    std::vector<std::uint8_t> payload(msg.begin(), msg.end());
    vel_tracks[index]->pushFrame(payload);
  };

  // ---- Terminal setup ----
  enableRawMode();

  std::printf("\n  Servo Controller (hold key to move)\n");
  std::printf("  ────────────────────────────────────\n");
  std::printf("  a / d  = pan  +/- %.0f deg/s\n", kVelocityMagnitude);
  std::printf("  w / s  = tilt +/- %.0f deg/s\n", kVelocityMagnitude);
  std::printf("  q      = quit\n\n");

  auto last_print = Clock::now();
  auto last_send = Clock::now();
  constexpr auto kPrintInterval = std::chrono::milliseconds(500);
  double prev_printed[SERVO_COUNT] = {0.0, 0.0};

  while (g_running.load()) {
    int key = readKeyNonBlocking();

    if (key != -1) {
      auto now_key = Clock::now();

      switch (key) {
      case 'a':
      case 'A':
        velocities[0] = +kVelocityMagnitude;
        last_key_time[0] = now_key;
        key_active[0] = true;
        break;
      case 'd':
      case 'D':
        velocities[0] = -kVelocityMagnitude;
        last_key_time[0] = now_key;
        key_active[0] = true;
        break;
      case 'w':
      case 'W':
        velocities[1] = +kVelocityMagnitude;
        last_key_time[1] = now_key;
        key_active[1] = true;
        break;
      case 's':
      case 'S':
        velocities[1] = -kVelocityMagnitude;
        last_key_time[1] = now_key;
        key_active[1] = true;
        break;
      case 'q':
      case 'Q':
        g_running.store(false);
        continue;
      default:
        break;
      }
    }

    auto now = Clock::now();

    // Zero velocity for any axis whose key hasn't repeated recently.
    for (int i = 0; i < SERVO_COUNT; ++i) {
      if (key_active[i] && (now - last_key_time[i]) >= kKeyHoldTimeout) {
        velocities[i] = 0.0;
        key_active[i] = false;
      }
    }

    // Resend current velocities periodically so servo_control's 300ms
    // timeout doesn't expire between key-repeat events.
    if (now - last_send >= kResendInterval) {
      last_send = now;
      for (int i = 0; i < SERVO_COUNT; ++i)
        sendVelocity(i);

      if (velocities[0] != prev_printed[0] ||
          velocities[1] != prev_printed[1]) {
        std::printf("  cmd -> pan: %+7.1f deg/s  tilt: %+7.1f deg/s\n",
                    velocities[0], velocities[1]);
        prev_printed[0] = velocities[0];
        prev_printed[1] = velocities[1];
      }
    }

    // Print state every 0.5s
    if (now - last_print >= kPrintInterval) {
      last_print = now;
      for (int i = 0; i < SERVO_COUNT; ++i) {
        std::lock_guard<std::mutex> lock(servo_states[i].mu);
        if (servo_states[i].received) {
          auto &s = servo_states[i].last_state;
          std::printf("  [%s] pos: %6.1f deg  speed: %+7.1f deg/s  "
                      "load: %d  temp: %d C\n",
                      kServoNames[i], s.value("position_deg", 0.0),
                      s.value("speed_deg_s", 0.0), s.value("load", 0),
                      s.value("temperature_c", 0));
        } else {
          std::printf("  [%s] (no state received)\n", kServoNames[i]);
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  // ---- Stop servos before exit ----
  velocities[0] = 0.0;
  velocities[1] = 0.0;
  for (int i = 0; i < SERVO_COUNT; ++i)
    sendVelocity(i);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // ---- Cleanup ----
  disableRawMode();

  for (int i = 0; i < SERVO_COUNT; ++i) {
    const auto track_name = makeTrackName(i, "state");
    LK_LOG_INFO("[controller] Unsubscribing from {}/{}", remote_identity,
                track_name);
    bridge.clearOnDataFrameCallback(remote_identity, track_name);
    LK_LOG_INFO("[controller] Resetting velocity track {}", track_name);
    vel_tracks[i].reset();
  }

  bridge.disconnect();
  std::printf("[controller] Done.\n");
  return 0;
}
