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

/// @file bridge_room_delegate.h
/// @brief Internal composing RoomDelegate that intercepts events for
///        LiveKitBridge and forwards all events to an optional user delegate.

#pragma once

#include "livekit/room_delegate.h"

namespace livekit_bridge {

class LiveKitBridge;

/**
 * Composing RoomDelegate that sits between the Room and an optional
 * user-supplied delegate.
 *
 * For events the bridge needs (track subscribe/unsubscribe), internal
 * logic runs first, then the event is forwarded to the user delegate.
 * All other events are forwarded directly. Not part of the public API.
 */
class BridgeRoomDelegate : public livekit::RoomDelegate {
public:
  explicit BridgeRoomDelegate(LiveKitBridge &bridge,
                              livekit::RoomDelegate *user_delegate = nullptr)
      : bridge_(bridge), user_delegate_(user_delegate) {}

  // -- Events with bridge-internal logic (+ forwarding) ------------------

  void onTrackSubscribed(livekit::Room &room,
                         const livekit::TrackSubscribedEvent &ev) override;

  void onTrackUnsubscribed(livekit::Room &room,
                           const livekit::TrackUnsubscribedEvent &ev) override;

  // -- Pure forwarding overrides -----------------------------------------

  void onParticipantConnected(
      livekit::Room &room,
      const livekit::ParticipantConnectedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onParticipantConnected(room, ev);
  }

  void onParticipantDisconnected(
      livekit::Room &room,
      const livekit::ParticipantDisconnectedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onParticipantDisconnected(room, ev);
  }

  void
  onLocalTrackPublished(livekit::Room &room,
                        const livekit::LocalTrackPublishedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onLocalTrackPublished(room, ev);
  }

  void onLocalTrackUnpublished(
      livekit::Room &room,
      const livekit::LocalTrackUnpublishedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onLocalTrackUnpublished(room, ev);
  }

  void onLocalTrackSubscribed(
      livekit::Room &room,
      const livekit::LocalTrackSubscribedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onLocalTrackSubscribed(room, ev);
  }

  void onTrackPublished(livekit::Room &room,
                        const livekit::TrackPublishedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onTrackPublished(room, ev);
  }

  void onTrackUnpublished(livekit::Room &room,
                          const livekit::TrackUnpublishedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onTrackUnpublished(room, ev);
  }

  void onTrackSubscriptionFailed(
      livekit::Room &room,
      const livekit::TrackSubscriptionFailedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onTrackSubscriptionFailed(room, ev);
  }

  void onTrackMuted(livekit::Room &room,
                    const livekit::TrackMutedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onTrackMuted(room, ev);
  }

  void onTrackUnmuted(livekit::Room &room,
                      const livekit::TrackUnmutedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onTrackUnmuted(room, ev);
  }

  void onActiveSpeakersChanged(
      livekit::Room &room,
      const livekit::ActiveSpeakersChangedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onActiveSpeakersChanged(room, ev);
  }

  void
  onRoomMetadataChanged(livekit::Room &room,
                        const livekit::RoomMetadataChangedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onRoomMetadataChanged(room, ev);
  }

  void onRoomSidChanged(livekit::Room &room,
                        const livekit::RoomSidChangedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onRoomSidChanged(room, ev);
  }

  void onRoomUpdated(livekit::Room &room,
                     const livekit::RoomUpdatedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onRoomUpdated(room, ev);
  }

  void onRoomMoved(livekit::Room &room,
                   const livekit::RoomMovedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onRoomMoved(room, ev);
  }

  void onParticipantMetadataChanged(
      livekit::Room &room,
      const livekit::ParticipantMetadataChangedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onParticipantMetadataChanged(room, ev);
  }

  void onParticipantNameChanged(
      livekit::Room &room,
      const livekit::ParticipantNameChangedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onParticipantNameChanged(room, ev);
  }

  void onParticipantAttributesChanged(
      livekit::Room &room,
      const livekit::ParticipantAttributesChangedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onParticipantAttributesChanged(room, ev);
  }

  void onParticipantEncryptionStatusChanged(
      livekit::Room &room,
      const livekit::ParticipantEncryptionStatusChangedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onParticipantEncryptionStatusChanged(room, ev);
  }

  void onConnectionQualityChanged(
      livekit::Room &room,
      const livekit::ConnectionQualityChangedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onConnectionQualityChanged(room, ev);
  }

  void onConnectionStateChanged(
      livekit::Room &room,
      const livekit::ConnectionStateChangedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onConnectionStateChanged(room, ev);
  }

  void onDisconnected(livekit::Room &room,
                      const livekit::DisconnectedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onDisconnected(room, ev);
  }

  void onReconnecting(livekit::Room &room,
                      const livekit::ReconnectingEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onReconnecting(room, ev);
  }

  void onReconnected(livekit::Room &room,
                     const livekit::ReconnectedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onReconnected(room, ev);
  }

  void onE2eeStateChanged(livekit::Room &room,
                          const livekit::E2eeStateChangedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onE2eeStateChanged(room, ev);
  }

  void onRoomEos(livekit::Room &room,
                 const livekit::RoomEosEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onRoomEos(room, ev);
  }

  void onUserPacketReceived(livekit::Room &room,
                            const livekit::UserDataPacketEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onUserPacketReceived(room, ev);
  }

  void onSipDtmfReceived(livekit::Room &room,
                         const livekit::SipDtmfReceivedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onSipDtmfReceived(room, ev);
  }

  void onDataStreamHeaderReceived(
      livekit::Room &room,
      const livekit::DataStreamHeaderReceivedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onDataStreamHeaderReceived(room, ev);
  }

  void onDataStreamChunkReceived(
      livekit::Room &room,
      const livekit::DataStreamChunkReceivedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onDataStreamChunkReceived(room, ev);
  }

  void onDataStreamTrailerReceived(
      livekit::Room &room,
      const livekit::DataStreamTrailerReceivedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onDataStreamTrailerReceived(room, ev);
  }

  void onDataChannelBufferedAmountLowThresholdChanged(
      livekit::Room &room,
      const livekit::DataChannelBufferedAmountLowThresholdChangedEvent &ev)
      override {
    if (user_delegate_)
      user_delegate_->onDataChannelBufferedAmountLowThresholdChanged(room, ev);
  }

  void onByteStreamOpened(livekit::Room &room,
                          const livekit::ByteStreamOpenedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onByteStreamOpened(room, ev);
  }

  void onTextStreamOpened(livekit::Room &room,
                          const livekit::TextStreamOpenedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onTextStreamOpened(room, ev);
  }

  void
  onParticipantsUpdated(livekit::Room &room,
                        const livekit::ParticipantsUpdatedEvent &ev) override {
    if (user_delegate_)
      user_delegate_->onParticipantsUpdated(room, ev);
  }

private:
  LiveKitBridge &bridge_;
  livekit::RoomDelegate *user_delegate_;
};

} // namespace livekit_bridge
