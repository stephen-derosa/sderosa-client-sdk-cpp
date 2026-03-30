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
 * UserTimestampedVideoConsumer
 *
 * Receives remote camera frames via Room::setOnVideoFrameEventCallback() and
 * logs any VideoFrameMetadata::user_timestamp_us values that arrive.
 */

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>

#include "livekit/livekit.h"

using namespace livekit;

namespace {

std::atomic<bool> g_running{true};

void handleSignal(int) { g_running.store(false); }

std::string getenvOrEmpty(const char *name) {
  const char *value = std::getenv(name);
  return value ? std::string(value) : std::string{};
}

std::string
formatUserTimestamp(const std::optional<VideoFrameMetadata> &metadata) {
  if (!metadata || !metadata->user_timestamp_us.has_value()) {
    return "n/a";
  }

  return std::to_string(*metadata->user_timestamp_us);
}

void printUsage(const char *program) {
  std::cerr << "Usage:\n"
            << "  " << program << " <ws-url> <token>\n"
            << "or:\n"
            << "  LIVEKIT_URL=... LIVEKIT_TOKEN=... " << program << "\n";
}

class UserTimestampedVideoConsumerDelegate : public RoomDelegate {
public:
  explicit UserTimestampedVideoConsumerDelegate(Room &room) : room_(room) {}

  void registerExistingParticipants() {
    for (const auto &participant : room_.remoteParticipants()) {
      if (participant) {
        registerRemoteCameraCallback(participant->identity());
      }
    }
  }

  void onParticipantConnected(Room &,
                              const ParticipantConnectedEvent &event) override {
    if (!event.participant) {
      return;
    }

    std::cout << "[consumer] participant connected: "
              << event.participant->identity() << "\n";
    registerRemoteCameraCallback(event.participant->identity());
  }

  void onParticipantDisconnected(
      Room &, const ParticipantDisconnectedEvent &event) override {
    if (!event.participant) {
      return;
    }

    const std::string identity = event.participant->identity();
    room_.clearOnVideoFrameCallback(identity, TrackSource::SOURCE_CAMERA);

    {
      std::lock_guard<std::mutex> lock(mutex_);
      registered_identities_.erase(identity);
    }

    std::cout << "[consumer] participant disconnected: " << identity << "\n";
  }

private:
  void registerRemoteCameraCallback(const std::string &identity) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!registered_identities_.insert(identity).second) {
        return;
      }
    }

    VideoStream::Options stream_options;
    stream_options.format = VideoBufferType::RGBA;

    room_.setOnVideoFrameEventCallback(
        identity, TrackSource::SOURCE_CAMERA,
        [identity](const VideoFrameEvent &event) {
          std::cout << "[consumer] from=" << identity
                    << " size=" << event.frame.width() << "x"
                    << event.frame.height()
                    << " capture_ts_us=" << event.timestamp_us
                    << " user_ts_us=" << formatUserTimestamp(event.metadata)
                    << " rotation=" << static_cast<int>(event.rotation) << "\n";
        },
        stream_options);

    std::cout << "[consumer] listening for camera frames from " << identity
              << "\n";
  }

  Room &room_;
  std::mutex mutex_;
  std::unordered_set<std::string> registered_identities_;
};

} // namespace

int main(int argc, char *argv[]) {
  std::string url = getenvOrEmpty("LIVEKIT_URL");
  std::string token = getenvOrEmpty("LIVEKIT_TOKEN");

  if (argc >= 3) {
    url = argv[1];
    token = argv[2];
  }

  if (url.empty() || token.empty()) {
    printUsage(argv[0]);
    return 1;
  }

  std::signal(SIGINT, handleSignal);
#ifdef SIGTERM
  std::signal(SIGTERM, handleSignal);
#endif

  livekit::initialize(livekit::LogLevel::Info, livekit::LogSink::kConsole);
  int exit_code = 0;

  {
    Room room;
    RoomOptions options;
    options.auto_subscribe = true;
    options.dynacast = false;

    UserTimestampedVideoConsumerDelegate delegate(room);
    room.setDelegate(&delegate);

    std::cout << "[consumer] connecting to " << url << "\n";
    if (!room.Connect(url, token, options)) {
      std::cerr << "[consumer] failed to connect\n";
      exit_code = 1;
    } else {
      std::cout << "[consumer] connected as "
                << room.localParticipant()->identity() << " to room '"
                << room.room_info().name << "'\n";

      delegate.registerExistingParticipants();

      while (g_running.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }

      for (const auto &participant : room.remoteParticipants()) {
        if (participant) {
          room.clearOnVideoFrameCallback(participant->identity(),
                                         TrackSource::SOURCE_CAMERA);
        }
      }
    }

    room.setDelegate(nullptr);
  }

  livekit::shutdown();
  return exit_code;
}
