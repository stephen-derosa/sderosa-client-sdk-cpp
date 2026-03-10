/*
 * Copyright 2025 LiveKit
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

#include "../common/test_common.h"

#include <iterator>
#include <memory>
#include <vector>

namespace livekit {
namespace test {

namespace {

std::vector<std::uint8_t> makeSharedKey() {
  static constexpr std::uint8_t kSharedKey[] = {
      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x61,
      0x62, 0x63, 0x64, 0x65, 0x66, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
      0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50};
  return std::vector<std::uint8_t>(std::begin(kSharedKey), std::end(kSharedKey));
}

RoomOptions makeEncryptedRoomOptions(const std::vector<std::uint8_t> &shared_key) {
  RoomOptions options;
  options.auto_subscribe = true;

  E2EEOptions e2ee;
  e2ee.encryption_type = EncryptionType::GCM;
  e2ee.key_provider_options.shared_key = shared_key;
  options.encryption = e2ee;

  return options;
}

} // namespace

class EncryptionIntegrationTest : public LiveKitTestBase {};

TEST_F(EncryptionIntegrationTest, ConnectWithoutEncryptionHasNoManager) {
  if (!config_.available) {
    GTEST_SKIP() << "LIVEKIT_URL, LIVEKIT_CALLER_TOKEN, and "
                    "LIVEKIT_RECEIVER_TOKEN not set";
  }

  Room room;
  RoomOptions options;
  ASSERT_TRUE(room.Connect(config_.url, config_.caller_token, options));
  EXPECT_EQ(room.e2eeManager(), nullptr)
      << "E2EE manager must be null when encryption options are not set";
}

TEST_F(EncryptionIntegrationTest, ConnectWithEncryptionInitializesManager) {
  if (!config_.available) {
    GTEST_SKIP() << "LIVEKIT_URL, LIVEKIT_CALLER_TOKEN, and "
                    "LIVEKIT_RECEIVER_TOKEN not set";
  }

  const std::vector<std::uint8_t> shared_key = makeSharedKey();
  RoomOptions encrypted_options = makeEncryptedRoomOptions(shared_key);

  auto receiver_room = std::make_unique<Room>();
  ASSERT_TRUE(receiver_room->Connect(config_.url, config_.receiver_token,
                                     encrypted_options));

  auto caller_room = std::make_unique<Room>();
  ASSERT_TRUE(
      caller_room->Connect(config_.url, config_.caller_token, encrypted_options));

  ASSERT_NE(receiver_room->e2eeManager(), nullptr);
  ASSERT_NE(caller_room->e2eeManager(), nullptr);
  ASSERT_NE(receiver_room->e2eeManager()->keyProvider(), nullptr);
  ASSERT_NE(caller_room->e2eeManager()->keyProvider(), nullptr);

  EXPECT_EQ(receiver_room->e2eeManager()->keyProvider()->exportSharedKey(0),
            shared_key);
  EXPECT_EQ(caller_room->e2eeManager()->keyProvider()->exportSharedKey(0),
            shared_key);

  caller_room->e2eeManager()->setEnabled(false);
  EXPECT_FALSE(caller_room->e2eeManager()->enabled());
  caller_room->e2eeManager()->setEnabled(true);
  EXPECT_TRUE(caller_room->e2eeManager()->enabled());

  EXPECT_NO_THROW({
    auto cryptors = caller_room->e2eeManager()->frameCryptors();
    (void)cryptors;
  });
}

} // namespace test
} // namespace livekit
