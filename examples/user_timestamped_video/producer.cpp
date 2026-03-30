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
 * UserTimestampedVideoProducer
 *
 * Publishes a synthetic camera track and stamps each frame with
 * VideoCaptureOptions::metadata.user_timestamp_us.
 */

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "livekit/livekit.h"

using namespace livekit;

namespace {

constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 360;
constexpr int kFrameIntervalMs = 200;

std::atomic<bool> g_running{true};

void handleSignal(int) { g_running.store(false); }

std::string getenvOrEmpty(const char *name) {
  const char *value = std::getenv(name);
  return value ? std::string(value) : std::string{};
}

std::uint64_t nowEpochUs() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

void fillFrame(VideoFrame &frame, std::uint32_t frame_index) {
  const std::uint8_t blue = static_cast<std::uint8_t>((frame_index * 7) % 255);
  const std::uint8_t green =
      static_cast<std::uint8_t>((frame_index * 13) % 255);
  const std::uint8_t red = static_cast<std::uint8_t>((frame_index * 29) % 255);

  std::uint8_t *data = frame.data();
  for (std::size_t i = 0; i < frame.dataSize(); i += 4) {
    data[i + 0] = blue;
    data[i + 1] = green;
    data[i + 2] = red;
    data[i + 3] = 255;
  }
}

void printUsage(const char *program) {
  std::cerr << "Usage:\n"
            << "  " << program << " <ws-url> <token>\n"
            << "or:\n"
            << "  LIVEKIT_URL=... LIVEKIT_TOKEN=... " << program << "\n";
}

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

    std::cout << "[producer] connecting to " << url << "\n";
    if (!room.Connect(url, token, options)) {
      std::cerr << "[producer] failed to connect\n";
      exit_code = 1;
    } else {
      std::cout << "[producer] connected as "
                << room.localParticipant()->identity() << " to room '"
                << room.room_info().name << "'\n";

      auto source = std::make_shared<VideoSource>(kFrameWidth, kFrameHeight);
      auto track =
          LocalVideoTrack::createLocalVideoTrack("timestamped-camera", source);

      try {
        TrackPublishOptions publish_options;
        publish_options.source = TrackSource::SOURCE_CAMERA;
        publish_options.packet_trailer_features.user_timestamp = true;

        room.localParticipant()->publishTrack(track, publish_options);
        std::cout << "[producer] published camera track with "
                     "packet_trailer_features.user_timestamp enabled\n";

        VideoFrame frame = VideoFrame::create(kFrameWidth, kFrameHeight,
                                              VideoBufferType::BGRA);
        const auto capture_start = std::chrono::steady_clock::now();
        std::uint32_t frame_index = 0;
        auto next_frame_at = std::chrono::steady_clock::now();

        while (g_running.load(std::memory_order_relaxed)) {
          fillFrame(frame, frame_index);

          VideoCaptureOptions capture_options;
          capture_options.timestamp_us = static_cast<std::int64_t>(
              std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::steady_clock::now() - capture_start)
                  .count());
          capture_options.rotation = VideoRotation::VIDEO_ROTATION_0;
          capture_options.metadata = VideoFrameMetadata{};
          capture_options.metadata->user_timestamp_us = nowEpochUs();

          source->captureFrame(frame, capture_options);

          if (frame_index % 5 == 0) {
            std::cout << "[producer] frame=" << frame_index
                      << " capture_ts_us=" << capture_options.timestamp_us
                      << " user_ts_us="
                      << *capture_options.metadata->user_timestamp_us << "\n";
          }

          ++frame_index;
          next_frame_at += std::chrono::milliseconds(kFrameIntervalMs);
          std::this_thread::sleep_until(next_frame_at);
        }
      } catch (const std::exception &error) {
        std::cerr << "[producer] error: " << error.what() << "\n";
        exit_code = 1;
      }

      if (track->publication()) {
        room.localParticipant()->unpublishTrack(track->publication()->sid());
      }
    }
  }

  livekit::shutdown();
  return exit_code;
}
