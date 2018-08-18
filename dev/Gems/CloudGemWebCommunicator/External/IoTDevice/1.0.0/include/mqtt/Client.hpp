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
 * @file Client.hpp
 * @brief Contains the MQTT Client class
 *
 * Defines MQTT client wrapper using a Client Core instance. This is provided
 * for ease of use. Instead of separately having to define Core Client and adding
 * Actions to the client, applications can use this class directly.
 */

#pragma once

#include "util/Utf8String.hpp"

#include "ClientCore.hpp"

#include "mqtt/Connect.hpp"
#include "mqtt/Publish.hpp"
#include "mqtt/Subscribe.hpp"
#include "mqtt/ClientState.hpp"

namespace awsiotsdk {

	/**
	 * @brief MQTT Client Class
	 *
	 * Defining a class for the MQTT Client.
	 * This class is a wrapper on the Core Client and creates a Client Core instance with MQTT Actions
	 * It also provides APIs to perform MQTT operations directly on the Core Client instance
	 *
	 */
	AWS_API_EXPORT class MqttClient {
	protected:
		std::unique_ptr<ClientCore> p_client_core_;				///< Unique pointer to the Client Core instance
		std::shared_ptr<mqtt::ClientState> p_client_state_;		///< MQTT Client state

		/**
		 * @brief Constructor
		 *
		 * @param p_network_connection - Network connection to use with this MQTT Client instance
		 * @param mqtt_command_timeout - Command timeout in milliseconds for internal blocking operations (Reconnect and Resubscribe)
		 *
		 */
		MqttClient(std::shared_ptr<NetworkConnection> p_network_connection,
				   std::chrono::milliseconds mqtt_command_timeout);
	public:

		// Disabling default and copy constructors. Defining a virtual destructor
		// Client instances should not be copied to avoid possible Connection issues with two clients
		// using same connection data
		MqttClient() = delete;									// Delete Default constructor
		MqttClient(const MqttClient &) = delete;				// Delete Copy constructor
        // LY 2013 Fix
		MqttClient& operator=(const MqttClient &) = delete;	// Delete Copy assignment operator
        // LY 2013 Fix
		virtual ~MqttClient();

		/**
		 * @brief Create factory method. Returns a unique instance of MqttClient
		 *
		 * @param p_network_connection - Network connection to use with this MQTT Client instance
		 * @param mqtt_command_timeout - Command timeout in milliseconds for internal blocking operations (Reconnect and Resubscribe)
		 *
		 * @return std::unique_ptr<MqttClient> pointing to a unique MQTT client instance
		 */
		static std::unique_ptr<MqttClient> Create(std::shared_ptr<NetworkConnection> p_network_connection,
												  std::chrono::milliseconds mqtt_command_timeout);

		// Sync API
		/**
		 * @brief Perform Sync Connect
		 *
		 * Performs a Network and MQTT Connect operation in blocking mode. Action timeout here is the time for which
		 * the client waits for a response AFTER the request is sent.
		 *
		 * @param p_connect_packet - Connect packet to use for the operation
		 * @param action_reponse_timeout - Timeout in milliseconds within which response should be obtained after request is sent
		 *
		 * @return ResponseCode indicating status of request
		 */
		virtual ResponseCode Connect(std::chrono::milliseconds action_reponse_timeout, bool is_clean_session,
									 mqtt::Version mqtt_version, std::chrono::seconds keep_alive_timeout,
									 std::unique_ptr<Utf8String> p_client_id, std::unique_ptr<Utf8String> p_username,
									 std::unique_ptr<Utf8String> p_password,
									 std::unique_ptr<mqtt::WillOptions> p_will_msg);

		/**
		 * @brief Perform Sync Disconnect
		 *
		 * Performs a Network and MQTT Disconnect operation in blocking mode. Action timeout here is the time for which
		 * the client waits for a response AFTER the request is sent.
		 *
		 * @param p_disconnect_packet - Disconnect packet to use for the operation
		 * @param action_reponse_timeout - Timeout in milliseconds within which response should be obtained after request is sent
		 *
		 * @return ResponseCode indicating status of request
		 */
		virtual ResponseCode Disconnect(std::chrono::milliseconds action_reponse_timeout);

		/**
		 * @brief Perform Sync Publish
		 *
		 * Performs a MQTT Publish operation in blocking mode. Action timeout here is the time for which
		 * the client waits for a response AFTER the request is sent.
		 *
		 * @param p_publish_packet - Publish packet to use for the operation
		 * @param action_reponse_timeout - Timeout in milliseconds within which response should be obtained after request is sent
		 *
		 * @return ResponseCode indicating status of request
		 */
		virtual ResponseCode Publish(std::unique_ptr<Utf8String> p_topic_name, bool is_retained, bool is_duplicate,
									 mqtt::QoS qos, const util::String &payload,
									 std::chrono::milliseconds action_reponse_timeout);

		/**
		 * @brief Perform Sync Subscribe
		 *
		 * Performs a MQTT Subscribe operation in blocking mode. Action timeout here is the time for which
		 * the client waits for a response AFTER the request is sent.
		 *
		 * @param p_subscribe_packet - Subscribe packet to use for the operation
		 * @param action_reponse_timeout - Timeout in milliseconds within which response should be obtained after request is sent
		 *
		 * @return ResponseCode indicating status of request
		 */
		virtual ResponseCode Subscribe(util::Vector<std::shared_ptr<mqtt::Subscription>> subscription_list,
									   std::chrono::milliseconds action_reponse_timeout);

		/**
		 * @brief Perform Sync Unsubscribe
		 *
		 * Performs a MQTT Unsubscribe operation in blocking mode. Action timeout here is the time for which
		 * the client waits for a response AFTER the request is sent.
		 *
		 * @param p_unsubscribe_packet - Unsubscribe packet to use for the operation
		 * @param action_reponse_timeout - Timeout in milliseconds within which response should be obtained after request is sent
		 *
		 * @return ResponseCode indicating status of request
		 */
		virtual ResponseCode Unsubscribe(util::Vector<std::unique_ptr<Utf8String>> topic_list,
										 std::chrono::milliseconds action_reponse_timeout);

		// Async API

		/**
		 * @brief Perform Async Publish
		 *
		 * Performs a MQTT Publish operation in Async mode. In the case of QoS1 requests, packet ID obtained from this
		 * function can be used to match Ack to specific requests if needed. QoS0 requests do not have a corrosponding
		 * Ack message and we do not support QoS2 at this time. The request is queued up and in the case of QoS1,
		 * the Ack Handler is called if a PUBACK is received. If not, the handler is called with a ResponseCode
		 * indicating timeout
		 *
		 * @param p_publish_packet - Publish packet to use for the operation
		 * @param p_async_ack_handler - AsyncAck notification handler to be called when response for this request is processed
		 * @param packet_id_out - Packet ID assigned to outgoing packet
		 *
		 * @return ResponseCode indicating status of request
		 */
		virtual ResponseCode PublishAsync(std::unique_ptr<Utf8String> p_topic_name, bool is_retained, bool is_duplicate,
										  mqtt::QoS qos, const util::String &payload,
										  ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler,
										  uint16_t &packet_id_out);

		/**
		 * @brief Perform Async Subscribe
		 *
		 * Performs a MQTT Subscribe operation in Async mode. Packet ID obtained from this function can be
		 * used to match Ack to specific requests if needed. The Subscribe request is queued up and Client automatically
		 * activates Subscription if successful SUBACK is received. If not, the assigned Ack handler will be called
		 * with the corrosponding ResponseCode
		 *
		 * @param p_subscribe_packet - Subscribe packet to use for the operation
		 * @param p_async_ack_handler - AsyncAck notification handler to be called when response for this request is processed
		 * @param packet_id_out - Packet ID assigned to outgoing packet
		 *
		 * @return ResponseCode indicating status of request
		 */
		virtual ResponseCode SubscribeAsync(util::Vector<std::shared_ptr<mqtt::Subscription>> subscription_list,
											ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler,
											uint16_t &packet_id_out);

		/**
		 * @brief Perform Async Unsubscribe
		 *
		 * Performs a MQTT Unsubscribe operation in Async mode. Packet ID obtained from this function can be used to
		 * match Ack to specific requests if needed. The Unsubscribe request is queued up and Client automatically
		 * deactivates the subscription if successful UNSUBACK is received. If not, the assigned Ack handler will be
		 * called with the corrosponding ResponseCode
		 *
		 * @param p_unsubscribe_packet - Unsubscribe packet to use for the operation
		 * @param p_async_ack_handler - AsyncAck notification handler to be called when response for this request is processed
		 * @param packet_id_out - Packet ID assigned to outgoing packet
		 *
		 * @return ResponseCode indicating status of request
		 */
		virtual ResponseCode UnsubscribeAsync(util::Vector<std::unique_ptr<Utf8String>> topic_list,
											  ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler,
											  uint16_t &packet_id_out);

		/**
		 * @brief Check if Client is in Connected state
		 *
		 * @return boolean indicating connection status
		 */
		virtual bool IsConnected();

		virtual void SetAutoReconnectEnabled(bool value) {
			p_client_state_->SetAutoReconnectEnabled(value);
		}

		virtual bool IsAutoReconnectEnabled() {
			return p_client_state_->IsAutoReconnectEnabled();
		}

		virtual std::chrono::seconds GetMinReconnectBackoffTimeout();
		virtual void SetMinReconnectBackoffTimeout(std::chrono::seconds min_reconnect_backoff_timeout);

		virtual std::chrono::seconds GetMaxReconnectBackoffTimeout();
		virtual void SetMaxReconnectBackoffTimeout(std::chrono::seconds max_reconnect_backoff_timeout);
	};
}
