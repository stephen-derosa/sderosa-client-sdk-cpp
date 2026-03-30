# UserTimestampedVideo

This example is split into two executables:

- `UserTimestampedVideoProducer` publishes a synthetic camera track and stamps
  each frame with `VideoCaptureOptions::metadata.user_timestamp_us`.
- `UserTimestampedVideoConsumer` subscribes to remote camera frames with
  `Room::setOnVideoFrameEventCallback` and logs the received user timestamp
  metadata.

Run them in the same room with different participant identities:

```sh
LIVEKIT_URL=ws://localhost:7880 LIVEKIT_TOKEN=<producer-token> ./UserTimestampedVideoProducer
LIVEKIT_URL=ws://localhost:7880 LIVEKIT_TOKEN=<consumer-token> ./UserTimestampedVideoConsumer
```
