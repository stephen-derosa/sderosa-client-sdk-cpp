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
 * Minimal bridge example — connects to a LiveKit room, creates a data
 * track, and pushes a heartbeat message every 3 seconds.
 *
 * All published tracks from every remote participant are printed
 * alongside each heartbeat so you can see who is in the room.
 *
 * Usage:
 *   simple_bridge <ws-url> <token>
 *   LIVEKIT_URL=... LIVEKIT_TOKEN=... simple_bridge
 */

#include "livekit_bridge/livekit_bridge.h"
#include "lk_log.h"

#include "livekit/room.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

static std::atomic<bool> g_running{true};
static void handleSignal(int) { g_running.store(false); }

int main(int argc, char *argv[]) {
  std::string url, token;
  if (argc >= 3) {
    url = argv[1];
    token = argv[2];
  } else {
    const char *e = std::getenv("LIVEKIT_URL");
    if (e)
      url = e;
    e = std::getenv("LIVEKIT_TOKEN");
    if (e)
      token = e;
  }

  if (url.empty() || token.empty()) {
    std::cerr << "Usage: simple_bridge <ws-url> <token>\n"
              << "   or: LIVEKIT_URL=... LIVEKIT_TOKEN=... simple_bridge\n";
    return 1;
  }

  std::signal(SIGINT, handleSignal);

  livekit_bridge::LiveKitBridge bridge;
  LK_LOG_INFO("[simple_bridge] Connecting to {} ...", url);

  livekit::RoomOptions options;
  options.auto_subscribe = true;

  if (!bridge.connect(url, token, options)) {
    LK_LOG_ERROR("[simple_bridge] Failed to connect.");
    return 1;
  }
  LK_LOG_INFO("[simple_bridge] Connected.");

  auto data_track = bridge.createDataTrack("heartbeat");
  LK_LOG_INFO("[simple_bridge] Created data track '{}'.", data_track->name());

  LK_LOG_INFO(
      "[simple_bridge] Publishing heartbeat every 3 s. Ctrl-C to stop.");

  uint64_t seq = 0;
  while (g_running.load()) {
    std::string msg = "heartbeat seq=" + std::to_string(seq);

    std::vector<std::uint8_t> payload(msg.begin(), msg.end());
    if (!data_track->pushFrame(payload)) {
      LK_LOG_WARN("[simple_bridge] pushFrame failed (track released?).");
      break;
    }

    LK_LOG_INFO("[simple_bridge] Sent: {}", msg);
    ++seq;

    for (int i = 0; i < 30 && g_running.load(); ++i)
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  LK_LOG_INFO("[simple_bridge] Shutting down...");
  data_track.reset();
  bridge.disconnect();`
  LK_LOG_INFO("[simple_bridge] Done.");
  return 0;
}
