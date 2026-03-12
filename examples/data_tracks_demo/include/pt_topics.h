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

#ifndef EXAMPLES_DATA_TRACKS_DEMO_PT_TOPICS_H
#define EXAMPLES_DATA_TRACKS_DEMO_PT_TOPICS_H

namespace pt_topics {
constexpr char kControlCmdTrack[] = "control_cmd";
constexpr char kGyroStateTrack[] = "gyro.state";
constexpr char kPanStateTrack[] = "pan.state";
constexpr char kTiltStateTrack[] = "tilt.state";
constexpr char kAcquireControlRpc[] = "acquire_control";
} // namespace pt_topics

#endif // EXAMPLES_DATA_TRACKS_DEMO_PT_TOPICS_H
