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
 * @file Connect.hpp
 * @brief
 *
 */

#pragma once

#include "mqtt/ClientState.hpp"
#include "mqtt/Packet.hpp"

namespace awsiotsdk {
	namespace mqtt {
		/**
		 * @brief Define strongly typed enum for Connack Return Codes
		 */
		enum class ConnackReturnCode {
			CONNECTION_ACCEPTED = 0,
			UNACCEPTABLE_PROTOCOL_VERSION_ERROR = 1,
			IDENTIFIER_REJECTED_ERROR = 2,
			SERVER_UNAVAILABLE_ERROR = 3,
			BAD_USERDATA_ERROR = 4,
			NOT_AUTHORIZED_ERROR = 5
		};

		/**
		 * @brief Connect Packet Type
		 *
		 * Defines a type for MQTT Connect message
		 */
		class ConnectPacket : public Packet {
		protected:
			bool is_clean_session_;							///< MQTT clean session.  True = this session is to be treated as clean.  Previous server state is cleared and no information is retained from any previous connection
			unsigned char connect_flags_;					///< MQTT Connect flags byte
			std::chrono::seconds keep_alive_timeout_;		///< MQTT Keepalive timeout in seconds
			mqtt::Version mqtt_version_;					///< Desired MQTT version used during connection
			std::unique_ptr<Utf8String> p_protocol_id_;		///< The protocol ID for this connection
			std::unique_ptr<Utf8String> p_client_id_;		///< Pointer to a string defining the MQTT client ID (this needs to be unique \b per \b device across your AWS account)
			std::unique_ptr<WillOptions> p_will_msg_;		///< MQTT LWT parameters

			// Currently not supported by AWS IoT. Kept for future use
			//std::unique_ptr<Utf8String> p_username_;            ///< MQTT Username
			//std::unique_ptr<Utf8String> p_password_;            ///< MQTT Password

		public:
			// Ensure Default and Copy Constructors and Copy assignment operator are deleted
			// Use default move constructors and assignment operators
			// Default virtual destructor
			// Delete Default constructor
			ConnectPacket() = delete;
			// Delete Copy constructor
			ConnectPacket(const ConnectPacket &) = delete;
			// Default Move constructor
            // LY 2013 Fix
			// Delete Copy assignment operator
			ConnectPacket& operator=(const ConnectPacket &) = delete;
			// Default Move assignment operator
            // LY 2013 Fix
			// Default destructor
			virtual ~ConnectPacket() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param is_clean_session - Is this a clean session? Currently we do not support setting this to false
			 * @param mqtt_version - Which version of the MQTT protocol to use. Currently the only allowed value is 3.1.1
			 * @param p_client_id - Client ID to use to make the connection
			 * @param p_username - Username, currently unused in AWS IoT and will be ignored
			 * @param p_password - Password, currently unused in AWS IoT and will be ignored
			 * @param p_will_msg - MQTT Last Will and Testament message
			 */
			ConnectPacket(bool is_clean_session, mqtt::Version mqtt_version, std::chrono::seconds keep_alive_timeout, std::unique_ptr<Utf8String> p_client_id, std::unique_ptr<Utf8String> p_username, std::unique_ptr<Utf8String> p_password, std::unique_ptr<mqtt::WillOptions> p_will_msg);

			/**
			 * @brief Create Factory method
			 *
			 * @param is_clean_session - Is this a clean session? Currently we do not support setting this to false
			 * @param mqtt_version - Which version of the MQTT protocol to use. Currently the only allowed value is 3.1.1
			 * @param p_client_id - Client ID to use to make the connection
			 * @param p_username - Username, currently unused in AWS IoT
			 * @param p_password - Password, currently unused in AWS IoT
			 * @param p_will_msg - MQTT Last Will and Testament message
			 * @return nullptr on error, shared_ptr pointing to a created ConnectPacket instance if successful
			 */
			static std::shared_ptr<ConnectPacket> Create(bool is_clean_session, mqtt::Version mqtt_version, std::chrono::seconds keep_alive_timeout, std::unique_ptr<Utf8String> p_client_id, std::unique_ptr<Utf8String> p_username, std::unique_ptr<Utf8String> p_password, std::unique_ptr<mqtt::WillOptions> p_will_msg);

			/**
			 * @brief Serialize this packet into a String
			 * @return String containing serialized packet
			 */
			util::String ToString();

			/**
			 * @brief Get duration of Keep alive interval in seconds
			 * @return std::chrono::seconds Keep alive interval duration
			 */
			std::chrono::seconds GetKeepAliveTimeout() { return keep_alive_timeout_; }
		};

		/**
		 * @brief Disconnect Packet Type
		 *
		 * Defines a type for MQTT Disconnect message
		 */
		class DisconnectPacket : public Packet {
		public:
			// Ensure Default move and copy constructors and assignment operators are created
			// Default virtual destructor
			DisconnectPacket(const DisconnectPacket &) = default;
			// Default Move constructor
            // LY 2013 Fix
			// Default Copy assignment operator
			DisconnectPacket& operator=(const DisconnectPacket &) = default;
			// Default Move assignment operator
            // LY 2013 Fix
			// Default destructor
			virtual ~DisconnectPacket() = default;

			/**
			 * @brief Constructor
			 */
			DisconnectPacket();

			/**
			 * @brief Create Factory method
			 *
			 * @return nullptr on error, shared_ptr pointing to a created DisconnectPacket instance if successful
			 */
			static std::shared_ptr<DisconnectPacket> Create();

			/**
			 * @brief Serialize this packet into a String
			 * @return String containing serialized packet
			 */
			util::String ToString();
		};

		class PingreqPacket : public Packet {
		public:
			// Ensure Default move and copy constructors and assignment operators are created
			// Default virtual destructor
			// Default Copy constructor
			PingreqPacket(const PingreqPacket &) = default;
			// Default Move constructor
            // LY 2013 Fix
			// Default Copy assignment operator
			PingreqPacket& operator=(const PingreqPacket &) = default;
			// Default Move assignment operator
            // LY 2013 Fix
			// Default destructor
			virtual ~PingreqPacket() = default;

			/**
			 * @brief Constructor
			 */
			PingreqPacket();

			/**
			 * @brief Create Factory method
			 *
			 * @return nullptr on error, shared_ptr pointing to a created PingreqPacket instance if successful
			 */
			static std::shared_ptr<PingreqPacket> Create();

			/**
			 * @brief Serialize this packet into a String
			 * @return String containing serialized packet
			 */
			util::String ToString();
		};

		/**
		 * @brief Define a class for ConnectActionAsync
		 *
		 * This class defines an Asynchronous action for performing a MQTT Connect operation
		 */
		class ConnectActionAsync : public Action {
		protected:
			std::shared_ptr<ClientState> p_client_state_;	///< Shared Client State instance
		public:
			// Disabling default, move and copy constructors to match Action parent
			// Default virtual destructor
			ConnectActionAsync() = delete;
			// Default Copy constructor
			ConnectActionAsync(const ConnectActionAsync &) = delete;
			// Default Move constructor
			ConnectActionAsync(ConnectActionAsync &&) = delete;
			// Default Copy assignment operator
			ConnectActionAsync& operator=(const ConnectActionAsync &) = delete;
			// Default Move assignment operator
			ConnectActionAsync& operator=(ConnectActionAsync &&) = delete;
			// Default destructor
			virtual ~ConnectActionAsync() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param p_client_state - Shared Client State instance
			 */
			ConnectActionAsync(std::shared_ptr<ClientState> p_client_state);

			/**
			 * @brief Factory Create method
			 *
			 * @param p_client_state - Shared Client State instance
			 * @return nullptr on error, unique_ptr pointing to a created ConnectActionAsync instance if successful
			 */
			static std::unique_ptr<Action> Create(std::shared_ptr<ActionState> p_action_state);

			/**
			 * @brief Perform MQTT Connect Action in Async mode
			 *
			 * Performs the MQTT Connect Operation in Async mode. This also calls Connect on the Network Connection
			 * provided with the Perform Action call. If the Network connect call fails, the action will return with
			 * the ResponseCode returned by the Network Connect call. This does not wait for CONNACK to be received.
			 * CONNACK is handled separately in the HandleConnack function of NetworkReadAction. If the MQTT connection
			 * is already active, will not attempt another Connect and return with appropriate ResponseCode.
			 *
			 * @param p_network_connection - Network connection instance to use for performing this action
			 * @param p_action_data - Action data specific to this execution of the Action
			 * @return - ResponseCode indicating status of the operation
			 */
			ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection, std::shared_ptr<ActionData> p_action_data);
		};

		/**
		 * @brief Define a class for DisconnectActionAsync
		 *
		 * This class defines an Asynchronous action for performing a MQTT Puback operation
		 */
		class DisconnectActionAsync : public Action {
		protected:
			std::shared_ptr<ClientState> p_client_state_;	///< Shared Client State instance
		public:
			// Disabling default, move and copy constructors to match Action parent
			// Default virtual destructor
			DisconnectActionAsync() = delete;
			// Default Copy constructor
			DisconnectActionAsync(const DisconnectActionAsync &) = delete;
			// Default Move constructor
			DisconnectActionAsync(DisconnectActionAsync &&) = delete;
			// Default Copy assignment operator
			DisconnectActionAsync& operator=(const DisconnectActionAsync &) = delete;
			// Default Move assignment operator
			DisconnectActionAsync& operator=(DisconnectActionAsync &&) = delete;
			// Default destructor
			virtual ~DisconnectActionAsync() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param p_client_state - Shared Client State instance
			 */
			DisconnectActionAsync(std::shared_ptr<ClientState> p_client_state);

			/**
			 * @brief Factory Create method
			 *
			 * @param p_client_state - Shared Client State instance
			 * @return nullptr on error, unique_ptr pointing to a created DisconnectActionAsync instance if successful
			 */
			static std::unique_ptr<Action> Create(std::shared_ptr<ActionState> p_action_state);

			/**
			 * @brief Perform MQTT Disconnect Action in Async mode
			 *
			 * Performs the MQTT Disconnect Operation in Async mode. Also calls disconnect API of the provided
			 * network connection. If the client is already in disconnected state, will not attempt disconnect and
			 * return with appropriate ResponseCode.
			 *
			 * @param p_network_connection - Network connection instance to use for performing this action
			 * @param p_action_data - Action data specific to this execution of the Action
			 * @return - ResponseCode indicating status of the operation
			 */
			ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection, std::shared_ptr<ActionData> p_action_data);
		};

		/**
		 * @brief Define a class for KeepaliveActionRunner
		 *
		 * This class defines an action for performing a MQTT Keep Alive operation. This is meant to be run in a separate
		 * thread using ClientCore and will not do anything if called for one single execution using Perform Action.
		 */
		class KeepaliveActionRunner : public Action {
		protected:
			std::shared_ptr<ClientState> p_client_state_;	///< Shared Client State instance
		public:
			// Disabling default, move and copy constructors to match Action parent
			// Default virtual destructor
			KeepaliveActionRunner() = delete;
			// Default Copy constructor
			KeepaliveActionRunner(const KeepaliveActionRunner &) = delete;
			// Default Move constructor
			KeepaliveActionRunner(KeepaliveActionRunner &&) = delete;
			// Default Copy assignment operator
			KeepaliveActionRunner& operator=(const KeepaliveActionRunner &) = delete;
			// Default Move assignment operator
			KeepaliveActionRunner& operator=(KeepaliveActionRunner &&) = delete;
			// Default destructor
			virtual ~KeepaliveActionRunner() = default;

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param p_client_state - Shared Client State instance
			 */
			KeepaliveActionRunner(std::shared_ptr<ClientState> p_client_state);

			/**
			 * @brief Factory Create method
			 *
			 * @param p_client_state - Shared Client State instance
			 * @return nullptr on error, unique_ptr pointing to a created KeepaliveActionRunner instance if successful
			 */
			static std::unique_ptr<Action> Create(std::shared_ptr<ActionState> p_action_state);

			/**
			 * @brief Perform MQTT Keep Alive Action. Expects to run in a separate thread using ClientCore
			 *
			 * Performs the MQTT Keep Alive operation. Will send out Ping requests at Half the specified Keepalive
			 * interval and expect a response to be received before that same period passes again. If a response is not
			 * received during that time, assumes connection has been lost and initiates and performs a reconnect. Also
			 * resubscribes to any existing subscribed topics. Uses exponential backoff using minimum and maximum values
			 * defined in Client state.
			 *
			 * @param p_network_connection - Network connection instance to use for performing this action
			 * @param p_action_data - Action data specific to this execution of the Action
			 * @return - ResponseCode indicating status of the operation
			 */
			ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection, std::shared_ptr<ActionData> p_action_data);
		};
	}
}
