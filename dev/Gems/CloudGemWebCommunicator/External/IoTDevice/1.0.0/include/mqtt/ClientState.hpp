/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file ClientState.hpp
 * @brief
 *
 */

#pragma once

#include <atomic>

#include "util/Utf8String.hpp"
#include "util/memory/stl/Map.hpp"

#include "Action.hpp"
#include "ClientCore.hpp"

#include "mqtt/Common.hpp"

namespace awsiotsdk {
	namespace mqtt {
		class ClientState : public ClientCoreState {
		protected:

			bool is_session_present_;
			std::atomic_bool is_connected_;
			std::atomic_bool is_auto_reconnect_enabled_;
			std::atomic_bool is_auto_reconnect_required_;
			std::atomic_bool is_pingreq_pending_;

			uint16_t last_sent_packet_id_;

			std::chrono::seconds keep_alive_timeout_;
			std::chrono::seconds min_reconnect_backoff_timeout_;
			std::chrono::seconds max_reconnect_backoff_timeout_;
			std::chrono::milliseconds mqtt_command_timeout_;

			std::shared_ptr<ActionData> p_connect_data_;
		public:
			util::Map<util::String, std::shared_ptr<Subscription>> subscription_map_;

			// Rule of 5 stuff
			// Disable copying because class contains std::atomic<> types used for thread synchronization
			ClientState() = delete;									// Default constructor
			ClientState(const ClientState&) = delete;				// Delete Copy constructor
            // LY 2013 Fix
			ClientState& operator=(const ClientState&) = delete;	// Delete Copy assignment operator
            // LY 2013 Fix
			~ClientState() = default;								// Default destructor

			ClientState(std::chrono::milliseconds mqtt_command_timeout);
			static std::shared_ptr<ClientState> Create(std::chrono::milliseconds mqtt_command_timeout);

			bool IsSessionPresent() { return is_session_present_; }
			void SetSessionPresent(bool value) { is_session_present_ = value; }

			bool IsConnected() { return is_connected_; }
			void SetConnected(bool value) {
				is_connected_ = value;
				if(value) {
					is_auto_reconnect_required_ = false;
				}
				SetProcessQueuedActions(value);
			}

			bool IsAutoReconnectEnabled() { return is_auto_reconnect_enabled_; }
			void SetAutoReconnectEnabled(bool value) { is_auto_reconnect_enabled_ = value; }

			bool IsAutoReconnectRequired() { return is_auto_reconnect_required_; }
			void SetAutoReconnectRequired(bool value) { is_auto_reconnect_required_ = value; }

			bool IsPingreqPending() { return is_pingreq_pending_; }
			void SetPingreqPending(bool value) { is_pingreq_pending_ = value; }

			virtual uint16_t GetNextPacketId();
			virtual uint16_t GetNextActionId() { return GetNextPacketId(); }

			/**
			 * @brief Get duration of Keep alive interval in seconds
			 * @return std::chrono::seconds Keep alive interval duration
			 */
			std::chrono::seconds GetKeepAliveTimeout() { return keep_alive_timeout_; }
			void SetKeepAliveTimeout(std::chrono::seconds keep_alive_timeout) { keep_alive_timeout_ = keep_alive_timeout; }

			std::chrono::milliseconds GetMqttCommandTimeout() { return mqtt_command_timeout_; }
			void SetMqttCommandTimeout(std::chrono::milliseconds mqtt_command_timeout) { mqtt_command_timeout_ = mqtt_command_timeout; }

			std::chrono::seconds GetMinReconnectBackoffTimeout() { return min_reconnect_backoff_timeout_; }
			void SetMinReconnectBackoffTimeout(std::chrono::seconds min_reconnect_backoff_timeout) { min_reconnect_backoff_timeout_ = min_reconnect_backoff_timeout; }

			std::chrono::seconds GetMaxReconnectBackoffTimeout() { return max_reconnect_backoff_timeout_; }
			void SetMaxReconnectBackoffTimeout(std::chrono::seconds max_reconnect_backoff_timeout) { max_reconnect_backoff_timeout_ = max_reconnect_backoff_timeout; }

			std::shared_ptr<ActionData> GetAutoReconnectData() { return p_connect_data_; }
			void SetAutoReconnectData(std::shared_ptr<ActionData> p_connect_data) { p_connect_data_ = p_connect_data; }

			std::shared_ptr<Subscription> GetSubscription(util::String p_topic_name);

			std::shared_ptr<Subscription> SetSubscriptionPacketInfo(util::String p_topic_name, uint16_t packet_id, uint8_t index_in_packet);

			ResponseCode SetSubscriptionActive(uint16_t packet_id, uint8_t index_in_sub_packet, mqtt::QoS max_qos);

			ResponseCode RemoveSubscription(uint16_t packet_id, uint8_t index_in_sub_packet);

			ResponseCode RemoveAllSubscriptionsForPacketId(uint16_t packet_id);

			ResponseCode RemoveSubscription(util::String p_topic_name);
		};
	}
}

