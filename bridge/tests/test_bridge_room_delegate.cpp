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

/// @file test_bridge_room_delegate.cpp
/// @brief Unit tests for BridgeRoomDelegate composing/forwarding behaviour.

#include "bridge_room_delegate.h"

#include <gtest/gtest.h>
#include <livekit/room.h>
#include <livekit/room_delegate.h>
#include <livekit/room_event_types.h>
#include <livekit_bridge/livekit_bridge.h>

namespace livekit_bridge {
namespace test {

// ============================================================================
// RecordingDelegate -- counts how many times each method is called
// ============================================================================

class RecordingDelegate : public livekit::RoomDelegate {
public:
  int participant_connected = 0;
  int participant_disconnected = 0;
  int local_track_published = 0;
  int local_track_unpublished = 0;
  int local_track_subscribed = 0;
  int track_published = 0;
  int track_unpublished = 0;
  int track_subscribed = 0;
  int track_unsubscribed = 0;
  int track_subscription_failed = 0;
  int track_muted = 0;
  int track_unmuted = 0;
  int active_speakers_changed = 0;
  int room_metadata_changed = 0;
  int room_sid_changed = 0;
  int room_updated = 0;
  int room_moved = 0;
  int participant_metadata_changed = 0;
  int participant_name_changed = 0;
  int participant_attributes_changed = 0;
  int participant_encryption_status_changed = 0;
  int connection_quality_changed = 0;
  int connection_state_changed = 0;
  int disconnected = 0;
  int reconnecting = 0;
  int reconnected = 0;
  int e2ee_state_changed = 0;
  int room_eos = 0;
  int user_packet_received = 0;
  int sip_dtmf_received = 0;
  int data_stream_header_received = 0;
  int data_stream_chunk_received = 0;
  int data_stream_trailer_received = 0;
  int data_channel_buffered_amount_low = 0;
  int byte_stream_opened = 0;
  int text_stream_opened = 0;
  int participants_updated = 0;

  void onParticipantConnected(livekit::Room &, const livekit::ParticipantConnectedEvent &) override { ++participant_connected; }
  void onParticipantDisconnected(livekit::Room &, const livekit::ParticipantDisconnectedEvent &) override { ++participant_disconnected; }
  void onLocalTrackPublished(livekit::Room &, const livekit::LocalTrackPublishedEvent &) override { ++local_track_published; }
  void onLocalTrackUnpublished(livekit::Room &, const livekit::LocalTrackUnpublishedEvent &) override { ++local_track_unpublished; }
  void onLocalTrackSubscribed(livekit::Room &, const livekit::LocalTrackSubscribedEvent &) override { ++local_track_subscribed; }
  void onTrackPublished(livekit::Room &, const livekit::TrackPublishedEvent &) override { ++track_published; }
  void onTrackUnpublished(livekit::Room &, const livekit::TrackUnpublishedEvent &) override { ++track_unpublished; }
  void onTrackSubscribed(livekit::Room &, const livekit::TrackSubscribedEvent &) override { ++track_subscribed; }
  void onTrackUnsubscribed(livekit::Room &, const livekit::TrackUnsubscribedEvent &) override { ++track_unsubscribed; }
  void onTrackSubscriptionFailed(livekit::Room &, const livekit::TrackSubscriptionFailedEvent &) override { ++track_subscription_failed; }
  void onTrackMuted(livekit::Room &, const livekit::TrackMutedEvent &) override { ++track_muted; }
  void onTrackUnmuted(livekit::Room &, const livekit::TrackUnmutedEvent &) override { ++track_unmuted; }
  void onActiveSpeakersChanged(livekit::Room &, const livekit::ActiveSpeakersChangedEvent &) override { ++active_speakers_changed; }
  void onRoomMetadataChanged(livekit::Room &, const livekit::RoomMetadataChangedEvent &) override { ++room_metadata_changed; }
  void onRoomSidChanged(livekit::Room &, const livekit::RoomSidChangedEvent &) override { ++room_sid_changed; }
  void onRoomUpdated(livekit::Room &, const livekit::RoomUpdatedEvent &) override { ++room_updated; }
  void onRoomMoved(livekit::Room &, const livekit::RoomMovedEvent &) override { ++room_moved; }
  void onParticipantMetadataChanged(livekit::Room &, const livekit::ParticipantMetadataChangedEvent &) override { ++participant_metadata_changed; }
  void onParticipantNameChanged(livekit::Room &, const livekit::ParticipantNameChangedEvent &) override { ++participant_name_changed; }
  void onParticipantAttributesChanged(livekit::Room &, const livekit::ParticipantAttributesChangedEvent &) override { ++participant_attributes_changed; }
  void onParticipantEncryptionStatusChanged(livekit::Room &, const livekit::ParticipantEncryptionStatusChangedEvent &) override { ++participant_encryption_status_changed; }
  void onConnectionQualityChanged(livekit::Room &, const livekit::ConnectionQualityChangedEvent &) override { ++connection_quality_changed; }
  void onConnectionStateChanged(livekit::Room &, const livekit::ConnectionStateChangedEvent &) override { ++connection_state_changed; }
  void onDisconnected(livekit::Room &, const livekit::DisconnectedEvent &) override { ++disconnected; }
  void onReconnecting(livekit::Room &, const livekit::ReconnectingEvent &) override { ++reconnecting; }
  void onReconnected(livekit::Room &, const livekit::ReconnectedEvent &) override { ++reconnected; }
  void onE2eeStateChanged(livekit::Room &, const livekit::E2eeStateChangedEvent &) override { ++e2ee_state_changed; }
  void onRoomEos(livekit::Room &, const livekit::RoomEosEvent &) override { ++room_eos; }
  void onUserPacketReceived(livekit::Room &, const livekit::UserDataPacketEvent &) override { ++user_packet_received; }
  void onSipDtmfReceived(livekit::Room &, const livekit::SipDtmfReceivedEvent &) override { ++sip_dtmf_received; }
  void onDataStreamHeaderReceived(livekit::Room &, const livekit::DataStreamHeaderReceivedEvent &) override { ++data_stream_header_received; }
  void onDataStreamChunkReceived(livekit::Room &, const livekit::DataStreamChunkReceivedEvent &) override { ++data_stream_chunk_received; }
  void onDataStreamTrailerReceived(livekit::Room &, const livekit::DataStreamTrailerReceivedEvent &) override { ++data_stream_trailer_received; }
  void onDataChannelBufferedAmountLowThresholdChanged(livekit::Room &, const livekit::DataChannelBufferedAmountLowThresholdChangedEvent &) override { ++data_channel_buffered_amount_low; }
  void onByteStreamOpened(livekit::Room &, const livekit::ByteStreamOpenedEvent &) override { ++byte_stream_opened; }
  void onTextStreamOpened(livekit::Room &, const livekit::TextStreamOpenedEvent &) override { ++text_stream_opened; }
  void onParticipantsUpdated(livekit::Room &, const livekit::ParticipantsUpdatedEvent &) override { ++participants_updated; }
};

// ============================================================================
// Test fixture
// ============================================================================

class BridgeRoomDelegateTest : public ::testing::Test {
protected:
  LiveKitBridge bridge_;
  livekit::Room room_;
};

// ============================================================================
// 1. Forwarding to user delegate (pure-forwarding methods)
// ============================================================================

TEST_F(BridgeRoomDelegateTest, ForwardsParticipantConnected) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onParticipantConnected(room_, {});
  EXPECT_EQ(user.participant_connected, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsParticipantDisconnected) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onParticipantDisconnected(room_, {});
  EXPECT_EQ(user.participant_disconnected, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsLocalTrackPublished) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onLocalTrackPublished(room_, {});
  EXPECT_EQ(user.local_track_published, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsLocalTrackUnpublished) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onLocalTrackUnpublished(room_, {});
  EXPECT_EQ(user.local_track_unpublished, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsLocalTrackSubscribed) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onLocalTrackSubscribed(room_, {});
  EXPECT_EQ(user.local_track_subscribed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsTrackPublished) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onTrackPublished(room_, {});
  EXPECT_EQ(user.track_published, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsTrackUnpublished) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onTrackUnpublished(room_, {});
  EXPECT_EQ(user.track_unpublished, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsTrackSubscriptionFailed) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onTrackSubscriptionFailed(room_, {});
  EXPECT_EQ(user.track_subscription_failed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsTrackMuted) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onTrackMuted(room_, {});
  EXPECT_EQ(user.track_muted, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsTrackUnmuted) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onTrackUnmuted(room_, {});
  EXPECT_EQ(user.track_unmuted, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsActiveSpeakersChanged) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onActiveSpeakersChanged(room_, {});
  EXPECT_EQ(user.active_speakers_changed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsRoomMetadataChanged) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onRoomMetadataChanged(room_, {});
  EXPECT_EQ(user.room_metadata_changed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsRoomSidChanged) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onRoomSidChanged(room_, {});
  EXPECT_EQ(user.room_sid_changed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsRoomUpdated) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onRoomUpdated(room_, {});
  EXPECT_EQ(user.room_updated, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsRoomMoved) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onRoomMoved(room_, {});
  EXPECT_EQ(user.room_moved, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsParticipantMetadataChanged) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onParticipantMetadataChanged(room_, {});
  EXPECT_EQ(user.participant_metadata_changed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsParticipantNameChanged) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onParticipantNameChanged(room_, {});
  EXPECT_EQ(user.participant_name_changed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsParticipantAttributesChanged) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onParticipantAttributesChanged(room_, {});
  EXPECT_EQ(user.participant_attributes_changed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsParticipantEncryptionStatusChanged) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onParticipantEncryptionStatusChanged(room_, {});
  EXPECT_EQ(user.participant_encryption_status_changed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsConnectionQualityChanged) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onConnectionQualityChanged(room_, {});
  EXPECT_EQ(user.connection_quality_changed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsConnectionStateChanged) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onConnectionStateChanged(room_, {});
  EXPECT_EQ(user.connection_state_changed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsDisconnected) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onDisconnected(room_, {});
  EXPECT_EQ(user.disconnected, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsReconnecting) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onReconnecting(room_, {});
  EXPECT_EQ(user.reconnecting, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsReconnected) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onReconnected(room_, {});
  EXPECT_EQ(user.reconnected, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsE2eeStateChanged) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onE2eeStateChanged(room_, {});
  EXPECT_EQ(user.e2ee_state_changed, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsRoomEos) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onRoomEos(room_, {});
  EXPECT_EQ(user.room_eos, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsUserPacketReceived) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onUserPacketReceived(room_, {});
  EXPECT_EQ(user.user_packet_received, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsSipDtmfReceived) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onSipDtmfReceived(room_, {});
  EXPECT_EQ(user.sip_dtmf_received, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsDataStreamHeaderReceived) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onDataStreamHeaderReceived(room_, {});
  EXPECT_EQ(user.data_stream_header_received, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsDataStreamChunkReceived) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onDataStreamChunkReceived(room_, {});
  EXPECT_EQ(user.data_stream_chunk_received, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsDataStreamTrailerReceived) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onDataStreamTrailerReceived(room_, {});
  EXPECT_EQ(user.data_stream_trailer_received, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsDataChannelBufferedAmountLow) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onDataChannelBufferedAmountLowThresholdChanged(room_, {});
  EXPECT_EQ(user.data_channel_buffered_amount_low, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsByteStreamOpened) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onByteStreamOpened(room_, {});
  EXPECT_EQ(user.byte_stream_opened, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsTextStreamOpened) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onTextStreamOpened(room_, {});
  EXPECT_EQ(user.text_stream_opened, 1);
}

TEST_F(BridgeRoomDelegateTest, ForwardsParticipantsUpdated) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);
  delegate.onParticipantsUpdated(room_, {});
  EXPECT_EQ(user.participants_updated, 1);
}

// ============================================================================
// 2. Null user delegate safety
// ============================================================================

TEST_F(BridgeRoomDelegateTest, NullUserDelegateDoesNotCrash) {
  BridgeRoomDelegate delegate(bridge_, nullptr);

  EXPECT_NO_THROW({
    delegate.onParticipantConnected(room_, {});
    delegate.onParticipantDisconnected(room_, {});
    delegate.onLocalTrackPublished(room_, {});
    delegate.onLocalTrackUnpublished(room_, {});
    delegate.onLocalTrackSubscribed(room_, {});
    delegate.onTrackPublished(room_, {});
    delegate.onTrackUnpublished(room_, {});
    delegate.onTrackSubscriptionFailed(room_, {});
    delegate.onTrackMuted(room_, {});
    delegate.onTrackUnmuted(room_, {});
    delegate.onActiveSpeakersChanged(room_, {});
    delegate.onRoomMetadataChanged(room_, {});
    delegate.onRoomSidChanged(room_, {});
    delegate.onRoomUpdated(room_, {});
    delegate.onRoomMoved(room_, {});
    delegate.onParticipantMetadataChanged(room_, {});
    delegate.onParticipantNameChanged(room_, {});
    delegate.onParticipantAttributesChanged(room_, {});
    delegate.onParticipantEncryptionStatusChanged(room_, {});
    delegate.onConnectionQualityChanged(room_, {});
    delegate.onConnectionStateChanged(room_, {});
    delegate.onDisconnected(room_, {});
    delegate.onReconnecting(room_, {});
    delegate.onReconnected(room_, {});
    delegate.onE2eeStateChanged(room_, {});
    delegate.onRoomEos(room_, {});
    delegate.onUserPacketReceived(room_, {});
    delegate.onSipDtmfReceived(room_, {});
    delegate.onDataStreamHeaderReceived(room_, {});
    delegate.onDataStreamChunkReceived(room_, {});
    delegate.onDataStreamTrailerReceived(room_, {});
    delegate.onDataChannelBufferedAmountLowThresholdChanged(room_, {});
    delegate.onByteStreamOpened(room_, {});
    delegate.onTextStreamOpened(room_, {});
    delegate.onParticipantsUpdated(room_, {});
  });
}

TEST_F(BridgeRoomDelegateTest, NullUserDelegateInterceptedEventsDoNotCrash) {
  BridgeRoomDelegate delegate(bridge_, nullptr);

  // Default-constructed events have null track/participant/publication,
  // so bridge internal logic is skipped, and null user delegate is safe.
  EXPECT_NO_THROW({
    delegate.onTrackSubscribed(room_, {});
    delegate.onTrackUnsubscribed(room_, {});
  });
}

// ============================================================================
// 3. Intercepted events still forward to user delegate
// ============================================================================

TEST_F(BridgeRoomDelegateTest, TrackSubscribedForwardsToUserDelegate) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);

  // Default-constructed event has null fields, so bridge internal logic
  // is skipped, but forwarding should still happen.
  livekit::TrackSubscribedEvent ev{};
  delegate.onTrackSubscribed(room_, ev);
  EXPECT_EQ(user.track_subscribed, 1);
}

TEST_F(BridgeRoomDelegateTest, TrackUnsubscribedForwardsToUserDelegate) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);

  livekit::TrackUnsubscribedEvent ev{};
  delegate.onTrackUnsubscribed(room_, ev);
  EXPECT_EQ(user.track_unsubscribed, 1);
}

TEST_F(BridgeRoomDelegateTest,
       TrackSubscribedForwardsEvenWhenFieldsAreNull) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);

  // Explicitly set some fields to null to confirm bridge logic skips
  // but forwarding still fires.
  livekit::TrackSubscribedEvent ev{};
  ev.track = nullptr;
  ev.publication = nullptr;
  ev.participant = nullptr;

  delegate.onTrackSubscribed(room_, ev);
  EXPECT_EQ(user.track_subscribed, 1);
}

TEST_F(BridgeRoomDelegateTest,
       TrackUnsubscribedForwardsEvenWhenFieldsAreNull) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);

  livekit::TrackUnsubscribedEvent ev{};
  ev.publication = nullptr;
  ev.participant = nullptr;

  delegate.onTrackUnsubscribed(room_, ev);
  EXPECT_EQ(user.track_unsubscribed, 1);
}

// ============================================================================
// 4. Multiple invocations accumulate
// ============================================================================

TEST_F(BridgeRoomDelegateTest, MultipleCallsAccumulate) {
  RecordingDelegate user;
  BridgeRoomDelegate delegate(bridge_, &user);

  delegate.onDisconnected(room_, {});
  delegate.onDisconnected(room_, {});
  delegate.onDisconnected(room_, {});
  EXPECT_EQ(user.disconnected, 3);
}

// ============================================================================
// 5. Default constructor omits user delegate (nullptr)
// ============================================================================

TEST_F(BridgeRoomDelegateTest, DefaultConstructedHasNoUserDelegate) {
  BridgeRoomDelegate delegate(bridge_);

  // Should behave identically to explicit nullptr -- no crash
  EXPECT_NO_THROW({
    delegate.onParticipantConnected(room_, {});
    delegate.onTrackSubscribed(room_, {});
    delegate.onTrackUnsubscribed(room_, {});
    delegate.onDisconnected(room_, {});
  });
}

} // namespace test
} // namespace livekit_bridge
