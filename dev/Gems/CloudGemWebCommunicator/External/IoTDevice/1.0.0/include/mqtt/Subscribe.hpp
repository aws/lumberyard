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
 * @file Subscribe.hpp
 * @brief MQTT Subscribe and Unsubscribe Actions and Action data definitions for IoT Client
 *
 * Defines classes for perform MQTT Subscribe and Unsubscribe Actions in Async mode for the IoT Client.
 * Also defines the packet types used by these actions as well as the related Ack packet types.
 */

#pragma once

#include "util/Utf8String.hpp"
#include "util/memory/stl/Map.hpp"
#include "util/memory/stl/Vector.hpp"
#include "mqtt/Packet.hpp"
#include "mqtt/Publish.hpp"

namespace awsiotsdk {
	namespace mqtt {
		/**
		 * @brief Define a class for Subscribe Packet type
		 *
		 * This class defines the Packet type used by MQTT Subscribe requests
		 */
		class SubscribePacket : public Packet {
		public:
			// Public to avoid extra move/copy operations when in use by action
			util::Vector<std::shared_ptr<Subscription>> subscription_list_;	///< Vector containing subscriptions included in this packet

			// Ensure Default Constructor is deleted, default to move and copy constructors and assignment operators
			// Default virtual destructor
			// Delete Default constructor
			SubscribePacket() = delete;
			// Default Copy constructor
			SubscribePacket(const SubscribePacket &) = default;
			// Default Move constructor
            // LY 2013 Fix
			// Default Copy assignment operator
			SubscribePacket& operator=(const SubscribePacket &) = default;
			// Default Move assignment operator
            // LY 2013 Fix
			// Default destructor
			virtual ~SubscribePacket() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param subscription_list Vector of subscriptions to be included in this Subscribe packet
			 */
			SubscribePacket(util::Vector<std::shared_ptr<Subscription>> subscription_list);

			/**
			 * @brief Factory Create method
			 * @param subscription_list Vector of subscriptions to be included in this Subscribe packet
			 * @return nullptr on error, shared_ptr pointing to a created SubscribePacket instance if successful
			 */
			static std::shared_ptr<SubscribePacket> Create(util::Vector<std::shared_ptr<Subscription>> subscription_list);

			/**
			 * @brief Serialize this packet into a String
			 * @return String containing serialized packet
			 */
			util::String ToString();
		};

		/**
		 * @brief Define a class for Suback Packet type
		 *
		 * This class defines the Packet type used in MQTT to Acknowledge Subscribe requests
		 */
		class SubackPacket : public Packet {
		public:
			// Public to avoid extra move/copy operations when in use by action
			util::Vector<uint8_t> suback_list_;	///< ///< Vector containing subacks included in this packet

			// Ensure Default Constructor is deleted, default to move and copy constructors and assignment operators
			// Default virtual destructor
			// Delete Default constructor
			SubackPacket() = delete;
			// Default Copy constructor
			SubackPacket(const SubackPacket &) = default;
			// Default Move constructor
            // LY 2013 Fix
			// Default Copy assignment operator
			SubackPacket& operator=(const SubackPacket &) = default;
			// Default Move assignment operator
            // LY 2013 Fix
			// Default destructor
			virtual ~SubackPacket() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param buf Serialized version of the packet to parse
			 */
			SubackPacket(const util::Vector<unsigned char> &buf);

			/**
			 * @brief Factory Create method
			 * @param buf Serialized version of the packet to parse
			 * @return nullptr on error, shared_ptr pointing to a created SubackPacket instance if successful
			 */
			static std::shared_ptr<SubackPacket> Create(const util::Vector<unsigned char> &buf);

			/**
			 * @brief Serialize this packet into a String
			 * @return String containing serialized packet
			 */
			util::String ToString();
		};

		/**
		 * @brief Define a class for Unsubscribe Packet type
		 *
		 * This class defines the Packet type used by MQTT Unsubscribe requests
		 */
		class UnsubscribePacket : public Packet {
		public:
			// Public to avoid copying/returning reference in Unsubscribe Action
			util::Vector<std::unique_ptr<Utf8String>> topic_list_;
			// Ensure Default Constructor is deleted, default to move and copy constructors and assignment operators
			// Default virtual destructor
			// Delete Default constructor
			UnsubscribePacket() = delete;
			// Default Copy constructor
			UnsubscribePacket(const UnsubscribePacket &) = default;
			// Default Move constructor
            // LY 2013 Fix
			// Default Copy assignment operator
			UnsubscribePacket& operator=(const UnsubscribePacket &) = default;
			// Default Move assignment operator
            // LY 2013 Fix
			// Default destructor
			virtual ~UnsubscribePacket() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param topic_list Vector of topic names to be included in this Unsubscribe packet
			 */
			UnsubscribePacket(util::Vector<std::unique_ptr<Utf8String>> topic_list);

			/**
			 * @brief Factory Create method
			 * @param topic_list Vector of topic names to be included in this Unsubscribe packet
			 * @return nullptr on error, shared_ptr pointing to a created UnsubscribePacket instance if successful
			 */
			static std::shared_ptr<UnsubscribePacket> Create(util::Vector<std::unique_ptr<Utf8String>> topic_list);

			/**
			 * @brief Serialize this packet into a String
			 * @return String containing serialized packet
			 */
			util::String ToString();
		};

		/**
		 * @brief Define a class for Unsuback Packet type
		 *
		 * This class defines the Packet type used in MQTT to Acknowledge Unsubscribe requests
		 */
		class UnsubackPacket : public Packet {
		public:
			// Ensure Default Constructor is deleted, default to move and copy constructors and assignment operators
			// Default virtual destructor
			// Delete Default constructor
			UnsubackPacket() = delete;
			// Default Copy constructor
			UnsubackPacket(const UnsubackPacket &) = default;
			// Default Move constructor
            // LY 2013 Fix
			// Default Copy assignment operator
			UnsubackPacket& operator=(const UnsubackPacket &) = default;
			// Default Move assignment operator
            // LY 2013 Fix
			// Default destructor
			virtual ~UnsubackPacket() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param buf Serialized version of the packet to parse
			 */
			UnsubackPacket(const util::Vector<unsigned char> &buf);

			/**
			 * @brief Factory Create method
			 * @param buf Serialized version of the packet to parse
			 * @return nullptr on error, shared_ptr pointing to a created UnsubackPacket instance if successful
			 */
			static std::shared_ptr<UnsubackPacket> Create(const util::Vector<unsigned char> &buf);

			/**
			 * @brief Serialize this packet into a String
			 * @return String containing serialized packet
			 */
			util::String ToString();
		};

		/**
		 * @brief Define a class for SubscribeActionAsync
		 *
		 * This class defines an Asynchronous action for performing a MQTT Subscribe operation
		 */
		class SubscribeActionAsync : public Action {
		protected:
			std::shared_ptr<ClientState> p_client_state_;	///< Shared Client State instance
		public:
			// Disabling default, move and copy constructors to match Action parent
			// Default virtual destructor
			SubscribeActionAsync() = delete;
			// Default Copy constructor
			SubscribeActionAsync(const SubscribeActionAsync &) = delete;
			// Default Move constructor
			SubscribeActionAsync(SubscribeActionAsync &&) = delete;
			// Default Copy assignment operator
            // LY 2013 Fix
			SubscribeActionAsync& operator=(const SubscribeActionAsync &) = delete;
			// Default Move assignment operator
			SubscribeActionAsync& operator=(SubscribeActionAsync &&) = delete;
			// Default destructor
			virtual ~SubscribeActionAsync() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param p_client_state - Shared Client State instance
			 */
			SubscribeActionAsync(std::shared_ptr<ClientState> p_client_state);

			/**
			 * @brief Factory Create method
			 *
			 * @param p_client_state - Shared Client State instance
			 * @return nullptr on error, unique_ptr pointing to a created SubscribeActionAsync instance if successful
			 */
			static std::unique_ptr<Action> Create(std::shared_ptr<ActionState> p_action_state);

			/**
			 * @brief Perform MQTT Subscribe Action in Async mode
			 *
			 * Performs the MQTT Subscribe Operation in Async mode. Registers the Subscriptions in the subscribe packet
			 * and sets them as inactive. Will NOT wait for SUBACK. Whenever SUBACK is received by the Network Read
			 * operation, the Subscriptions will be activated/removed depending on response.
			 *
			 * @param p_network_connection - Network connection instance to use for performing this action
			 * @param p_action_data - Action data specific to this execution of the Action
			 * @return - ResponseCode indicating status of the operation
			 */
			ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection, std::shared_ptr<ActionData> p_action_data);
		};

		/**
		 * @brief Define a class for UnsubscribeActionAsync
		 *
		 * This class defines an Asynchronous action for performing a MQTT Unsubscribe operation
		 */
		class UnsubscribeActionAsync : public Action {
		protected:
			std::shared_ptr<ClientState> p_client_state_;	///< Shared Client State instance
		public:
			// Disabling default, move and copy constructors to match Action parent
			// Default virtual destructor
			UnsubscribeActionAsync() = delete;
			// Default Copy constructor
			UnsubscribeActionAsync(const UnsubscribeActionAsync &) = delete;
			// Default Move constructor
			UnsubscribeActionAsync(UnsubscribeActionAsync &&) = delete;
			// Default Copy assignment operator
            // LY 2013 Fix
			UnsubscribeActionAsync& operator=(const UnsubscribeActionAsync &) = delete;
			// Default Move assignment operator
			UnsubscribeActionAsync& operator=(UnsubscribeActionAsync &&) = delete;
			// Default destructor
			virtual ~UnsubscribeActionAsync() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param p_client_state - Shared Client State instance
			 */
			UnsubscribeActionAsync(std::shared_ptr<ClientState> p_client_state);

			/**
			 * @brief Factory Create method
			 *
			 * @param p_client_state - Shared Client State instance
			 * @return nullptr on error, unique_ptr pointing to a created UnsubscribeActionAsync instance if successful
			 */
			static std::unique_ptr<Action> Create(std::shared_ptr<ActionState> p_action_state);

			/**
			 * @brief Perform MQTT Unsubscribe Action in Async mode
			 *
			 * Performs the MQTT Unsubscribe Operation in Async mode. Does NOT deactivate or deregister active
			 * subscriptions. Does modify subscription information to keep track of which packet ID the unsuback request
			 * is using. Will NOT wait for UNSUBACK. Whenever UNSUBACK is received by the Network Read
			 * operation, the Subscriptions will be removed.
			 *
			 * @param p_network_connection - Network connection instance to use for performing this action
			 * @param p_action_data - Action data specific to this execution of the Action
			 * @return - ResponseCode indicating status of the operation
			 */
			ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection, std::shared_ptr<ActionData> p_action_data);
		};
	}
}
