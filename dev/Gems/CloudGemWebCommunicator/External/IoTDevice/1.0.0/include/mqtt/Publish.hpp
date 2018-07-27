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
 * @file Publish.hpp
 * @brief MQTT Publish and Puback Actions and Action data definitions for IoT Client
 *
 * Defines classes for perform MQTT Publish and Puback Actions in Async mode for the IoT Client.
 * Also defines the packet types used by these actions.
 */

#pragma once

#include "mqtt/ClientState.hpp"
#include "mqtt/Packet.hpp"

namespace awsiotsdk {
	namespace mqtt {
		/**
		 * @brief Publish Message Packet Type
		 *
		 * Defines a type for MQTT Publish messages. Used for both incoming and out going messages
		 */
		class PublishPacket : public Packet {
		protected:
			bool is_retained_;        ///< Retained messages are \b NOT supported by the AWS IoT Service at the time of this SDK release
			bool is_duplicate_;        ///< Is this message a duplicate QoS > 0 message?  Handled automatically by the MQTT client
			QoS qos_;                ///< Message Quality of Service
			std::unique_ptr<Utf8String> p_topic_name_;    ///< Topic Name this packet was published to
			util::String payload_;            ///< MQTT message payload
		public:
			// Ensure Default and Copy Constructors and Copy assignment operator are deleted
			// Use default move constructors and assignment operators
			// Default virtual destructor
			// Delete Default constructor
			PublishPacket() = delete;
			// Delete Copy constructor
			PublishPacket(const PublishPacket &) = delete;
			// Default Move constructor
            // LY 2013 Fix
			// Delete Copy assignment operator
			PublishPacket& operator=(const PublishPacket &) = delete;
			// Default Move assignment operator
            // LY 2013 Fix
			// Default destructor
			virtual ~PublishPacket() = default;

			/**
			 * @brief Constructor, Individual data
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param p_topic_name Topic name on which message is to be published
			 * @param is_retained Is retained flag
			 * @param is_duplicate Is duplicate message flag
			 * @param qos QoS to use for this message, QoS2 is not supported currently
			 * @param payload String containing payload to send with message. Can be zero length.
			 */
			PublishPacket(std::unique_ptr<Utf8String> p_topic_name, bool is_retained, bool is_duplicate, QoS qos, const util::String &payload);

			/**
			 * @brief Constructor, Deserializes data from buffer
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param buf Buffer containing packet data
			 * @param is_retained Is retained flag
			 * @param is_duplicate Is duplicate message flag
			 * @param qos QoS used by this message
			 */
			PublishPacket(const util::Vector<unsigned char> &buf, bool is_retained, bool is_duplicate, QoS qos);

			/**
			 * @brief Create Factory method using Individual data
			 *
			 * @param p_topic_name Topic name on which message is to be published
			 * @param is_retained Is retained flag
			 * @param is_duplicate Is duplicate message flag
			 * @param qos QoS to use for this message, QoS2 is not supported currently
			 * @param payload String containing payload to send with message. Can be zero length
			 * @return nullptr on error, shared_ptr pointing to a created PublishPacket instance if successful
			 */
			static std::shared_ptr<PublishPacket> Create(std::unique_ptr<Utf8String> p_topic_name, bool is_retained, bool is_duplicate, QoS qos, const util::String &payload);

			/**
			 * @brief Create Factory method which deserializes data from a buffer
			 *
			 * @param buf Buffer containing packet data
			 * @param is_retained Is retained flag
			 * @param is_duplicate Is duplicate message flag
			 * @param qos QoS used by this message
			 * @return nullptr on error, shared_ptr pointing to a created PublishPacket instance if successful
			 */
			static std::shared_ptr<PublishPacket> Create(const util::Vector<unsigned char> &buf, bool is_retained, bool is_duplicate, QoS qos);

			/**
			 * @brief Get the value of the Is Retained flag
			 * @return boolean indicating the value of the Is Retained flag
			 */
			bool IsRetained() { return is_retained_; }

			/**
			 * @brief Get the value of the Is Duplicate message flag
			 * @return boolean indicating the value of the Is Duplicate message flag
			 */
			bool IsDuplicate() { return is_duplicate_; }

			/**
			 * @brief Get String containing topic name for this message
			 * @return util::String with topic name
			 */
			util::String GetTopicName() { return p_topic_name_->ToStdString(); }

			/**
			 * @brief Get string containing Payload
			 * @return util::String with payload
			 */
			util::String GetPayload() { return payload_; }

			/**
			 * @brief Get length of the payload
			 * @return util::String with payload length
			 */
			size_t GetPayloadLen() { return payload_.length(); }

			/**
			 * @brief Serialize this packet into a String
			 * @return String containing serialized packet
			 */
			util::String ToString();

			QoS GetQoS() { return qos_; }
		};

		/**
		 * @brief Define a class for Puback Packet type
		 *
		 * This class defines the Packet type used in MQTT to Acknowledge Publish requests
		 */
		class PubackPacket : public Packet {
		public:
			// Ensure Default Constructor is deleted, default to move and copy constructors and assignment operators
			// Default virtual destructor
			// Delete Default constructor
			PubackPacket() = delete;
			// Default Copy constructor
			PubackPacket(const PubackPacket &) = default;
			// Default Move constructor
            // LY 2013 Fix
			// Default Copy assignment operator
			PubackPacket& operator=(const PubackPacket &) = default;
			// Default Move assignment operator
            // LY 2013 Fix
			// Default destructor
			virtual ~PubackPacket() = default;

			/**
			 * @brief Constructor
			 *
			 * @param packet_id Packet ID for this Puback
			 */
			PubackPacket(uint16_t packet_id);

			/**
			 * @brief Factory Create method
			 * @param packet_id Packet ID for this Puback
			 * @return nullptr on error, shared_ptr pointing to a created PubackPacket instance if successful
			 */
			static std::shared_ptr<PubackPacket> Create(uint16_t packet_id);

			/**
			 * @brief Serialize this packet into a String
			 * @return String containing serialized packet
			 */
			util::String ToString();
		};

		/**
		 * @brief Define a class for PublishActionAsync
		 *
		 * This class defines an Asynchronous action for performing a MQTT Publish operation
		 */
		class PublishActionAsync : public Action {
		protected:
			std::shared_ptr<ClientState> p_client_state_;	///< Shared Client State instance
		public:
			// Disabling default, move and copy constructors to match Action parent
			// Default virtual destructor
			PublishActionAsync() = delete;
			// Default Copy constructor
			PublishActionAsync(const PublishActionAsync &) = delete;
			// Default Move constructor
			PublishActionAsync(PublishActionAsync &&) = delete;
			// Default Copy assignment operator
			PublishActionAsync& operator=(const PublishActionAsync &) = delete;
			// Default Move assignment operator
			PublishActionAsync& operator=(PublishActionAsync &&) = delete;
			// Default destructor
			virtual ~PublishActionAsync() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param p_client_state - Shared Client State instance
			 */
			PublishActionAsync(std::shared_ptr<ClientState> p_client_state);

			/**
			 * @brief Factory Create method
			 *
			 * @param p_client_state - Shared Client State instance
			 * @return nullptr on error, unique_ptr pointing to a created PublishActionAsync instance if successful
			 */
			static std::unique_ptr<Action> Create(std::shared_ptr<ActionState> p_action_state);

			/**
			 * @brief Perform MQTT Publish Action in Async mode
			 *
			 * Performs the MQTT Publish Operation in Async mode. For QoS0 operations, the packet is written to the
			 * network layer and the operation returns. If the packet has QoS1, and a response handler is provided,
			 * the handler will be added to the Pending Acks list.
			 *
			 * @param p_network_connection - Network connection instance to use for performing this action
			 * @param p_action_data - Action data specific to this execution of the Action
			 * @return - ResponseCode indicating status of the operation
			 */
			ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection, std::shared_ptr<ActionData> p_action_data);
		};

		/**
		 * @brief Define a class for PubackActionAsync
		 *
		 * This class defines an Asynchronous action for performing a MQTT Puback operation
		 */
		class PubackActionAsync : public Action {
		protected:
			std::shared_ptr<ClientState> p_client_state_;	///< Shared Client State instance
		public:
			// Disabling default, move and copy constructors to match Action parent
			// Default virtual destructor
			PubackActionAsync() = delete;
			// Default Copy constructor
			PubackActionAsync(const PubackActionAsync &) = delete;
			// Default Move constructor
			PubackActionAsync(PubackActionAsync &&) = delete;
			// Default Copy assignment operator
			PubackActionAsync& operator=(const PubackActionAsync &) = delete;
			// Default Move assignment operator
			PubackActionAsync& operator=(PubackActionAsync &&) = delete;
			// Default destructor
			virtual ~PubackActionAsync() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param p_client_state - Shared Client State instance
			 */
			PubackActionAsync(std::shared_ptr<ClientState> p_client_state);

			/**
			 * @brief Factory Create method
			 *
			 * @param p_client_state - Shared Client State instance
			 * @return nullptr on error, unique_ptr pointing to a created PubackActionAsync instance if successful
			 */
			static std::unique_ptr<Action> Create(std::shared_ptr<ActionState> p_action_state);

			/**
			 * @brief Perform MQTT Puback Action in Async mode
			 *
			 * Performs the MQTT Puback Operation in Async mode. This action should be queued up by the HandlePublish
			 * function in NetworkRead action automatically whenever a QoS1 packet is received. We do not support QoS2
			 * at this time.
			 *
			 * @param p_network_connection - Network connection instance to use for performing this action
			 * @param p_action_data - Action data specific to this execution of the Action
			 * @return - ResponseCode indicating status of the operation
			 */
			ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection, std::shared_ptr<ActionData> p_action_data);
		};
	}
}
