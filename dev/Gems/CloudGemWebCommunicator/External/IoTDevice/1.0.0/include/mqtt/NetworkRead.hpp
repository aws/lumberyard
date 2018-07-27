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
 * @file NetworkRead.hpp
 * @brief
 *
 */

#pragma once

#include "util/memory/stl/Map.hpp"

#include "ResponseCode.hpp"
#include "Action.hpp"
#include "Connect.hpp"
#include "Publish.hpp"
#include "Subscribe.hpp"

#define MAX_NO_OF_REMAINING_LENGTH_BYTES 4

namespace awsiotsdk {
	namespace mqtt {

		/**
		 * @brief Define a class for NetworkReadActionRunner
		 *
		 * This class defines an Asynchronous action for performing a MQTT Read operation
		 * Can run both as a one time operation as well as in a separate thread
		 */
		class NetworkReadActionRunner : public Action {
		protected:
			std::shared_ptr<ClientState> p_client_state_;				///< Shared Client State instance
			std::shared_ptr<NetworkConnection> p_network_connection_;	///< Shared Network Connection instance

			std::atomic_bool is_waiting_for_connack_;	///< Is this waiting for connack?

			/**
			 * @brief Decode Remaining length from MQTT packet
			 *
			 * @param rem_len reference in which to store decoded length
			 *
			 * @return ResponseCode indicating status of request
			 */
			ResponseCode DecodeRemainingLength(size_t &rem_len);

			/**
			 * @brief Read MQTT Packet from buffer
			 *
			 * @param fixed_header_byte Reference to string in which Fixed header byte should be stored
			 * @param read_buf Reference to string in which the rest of the packet should be stored
			 *
			 * @return ResponseCode indicating status of request
			 */
			ResponseCode ReadPacketFromNetwork(unsigned char &fixed_header_byte, util::Vector<unsigned char> &read_buf);

			/**
			 * @brief Handle MQTT Connack packet
			 *
			 * @param read_buf Reference to string buffer containing the MQTT Connack payload
			 *
			 * @return ResponseCode indicating status of request
			 */
			ResponseCode HandleConnack(const util::Vector<unsigned char> &read_buf);

			/**
			 * @brief Handle MQTT Publish packet
			 *
			 * @param read_buf Reference to string buffer containing the MQTT Publish payload
			 * @param is_duplicate MQTT Is Duplicate message flag
			 * @param is_retained MQTT Is retained flag
			 * @param qos QoS of received Publish message
			 *
			 * @return ResponseCode indicating status of request
			 */
			ResponseCode HandlePublish(const util::Vector<unsigned char> &read_buf, bool is_duplicate, bool is_retained, QoS qos);

			/**
			 * @brief Handle MQTT Puback packet
			 *
			 * @param read_buf Reference to string buffer containing the MQTT Puback payload
			 *
			 * @return ResponseCode indicating status of request
			 */
			ResponseCode HandlePuback(const util::Vector<unsigned char> &read_buf);

			/**
			 * @brief Handle MQTT Suback packet
			 *
			 * @param read_buf Reference to string buffer containing the MQTT Suback payload
			 *
			 * @return ResponseCode indicating status of request
			 */
			ResponseCode HandleSuback(const util::Vector<unsigned char> &read_buf);

			/**
			 * @brief Handle MQTT Unsuback packet
			 *
			 * @param read_buf Reference to string buffer containing the MQTT Unsuback payload
			 *
			 * @return ResponseCode indicating status of request
			 */
			ResponseCode HandleUnsuback(const util::Vector<unsigned char> &read_buf);
		public:

			/**
			 * @brief Constructor
			 *
			 * @warning This constructor can throw exceptions, it is recommended to use Factory create method
			 * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
			 *
			 * @param p_client_state - Shared Client State instance
			 */
			NetworkReadActionRunner(std::shared_ptr<ClientState> p_client_state);

			/**
			 * @brief Factory Create method
			 *
			 * @param p_client_state - Shared Client State instance
			 * @return nullptr on error, unique_ptr pointing to a created NetworkReadActionRunner instance if successful
			 */
			static std::unique_ptr<Action> Create(std::shared_ptr<ActionState> p_action_state);

			/**
			 * @brief Perform Network Read Action in Async mode
			 *
			 * Performs a Network read to see if there is any incoming MQTT packet in the provided Network Connection's
			 * Read buffer. Can be run as a one time operation or as a Client Core thread.
			 *
			 * @param p_network_connection - Network connection instance to use for performing this action
			 * @param p_action_data - Action data specific to this execution of the Action
			 * @return - ResponseCode indicating status of the operation
			 */
			ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection, std::shared_ptr<ActionData> p_action_data);
		};
	}
}
