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
 * @file Action.hpp
 * @brief Action Base class and related definitions for IoT Client
 *
 * Defines a base class to be used by all Actions that can be run by the
 * IoT Client. Also contains definitions for related Action types like
 * ActionType, ActionState and ActionData
 */

#pragma once

#include <string>
#include <iostream>
#include <memory>
#include <atomic>

#include "util/Utf8String.hpp"
#include "util/threading/ThreadTask.hpp"

#include "ResponseCode.hpp"
#include "NetworkConnection.hpp"

#define DEFAULT_NETWORK_ACTION_THREAD_SLEEP_DURATION_MS 100

namespace awsiotsdk {
	/**
	 * @brief ActionType Enum Class
	 *
	 * Defines a strongly typed enum which specifies different Action Types.
	 * Actions can be registered in the core client for each of these Types.
	 */
	enum class ActionType {
		RESERVED_ACTION = 0,
		CORE_PROCESS_INBOUND = 1,
		CORE_PROCESS_OUTBOUND = 2,
		CONNECT = 3,
		DISCONNECT = 4,
		PUBLISH = 5,
		PUBACK = 6,
		PUBREC = 7,
		PUBREL = 8,
		PUBCOMP = 9,
		SUBSCRIBE = 10,
		READ_INCOMING = 11,
		KEEP_ALIVE = 12,
		PUBLISH_INBOUND = 13,
		UNSUBSCRIBE = 14,
		RECONNECT = 15
	};

	/**
	 * @brief Action State Class
	 *
	 * Defines an Action State class which is retained by each Action for its lifetime
	 * This is a pure virtual class, cannot be instantiated
	 */
	class ActionState {
	public:
		/**
		 * @brief Get Action ID of the next Action
		 *
		 * @return uint16_t Next Action ID
		 */
		virtual uint16_t GetNextActionId() = 0;

		// Rule of 5 stuff
		// Keeping defaults, explicitly mentioned to avoid conflicts with future C++ specifications
		// Must always be able to copy and move
		ActionState() = default;                                // Default constructor
		ActionState(const ActionState &) = delete;                // Copy constructor
		ActionState(ActionState &&) = delete;                    // Move constructor
		ActionState& operator=(const ActionState &) = delete;    // Copy assignment operator
		ActionState& operator=(ActionState &&) = delete;        // Move assignment operator
		virtual ~ActionState() = default;                        // Default destructor
	};

	/**
	 * @brief Action Data Class
	 *
	 * Defines an Action Data class which contains dynamic information to be used by the action
	 * Instance of concrete implementation of this class is passed as argument each time Perform Action is called
	 * This is a pure virtual class, cannot be instantiated
	 */
	class ActionData {
	public:
		/**
		 * Define a type for the Async Ack notification handler
		 * Clients can provide an instance of this to receive notification on status of Async API calls
		 */
		typedef std::function<void(uint16_t action_id, ResponseCode rc)> AsyncAckNotificationHandlerPtr;

		// Rule of 5 stuff
		// Since virtual destructor is present, derived classes will have to explicitly define defaults
		// Must always be able to copy and move
		ActionData() = default;                                    // Default constructor
		ActionData(const ActionData &) = default;                // Copy constructor
        // LY 2013 Fix
		ActionData& operator=(const ActionData &) = default;    // Copy assignment operator
        // LY 2013 Fix
		virtual ~ActionData() = default;                        // Default destructor

		AsyncAckNotificationHandlerPtr p_async_ack_handler_;	///< Handler to call when response is received for this action

		/**
		 * @brief Get ID of the current run of this Action
		 * @return uint16_t - Action ID
		 */
		virtual uint16_t GetActionId() = 0;

		/**
		 * @brief Set the Action ID for this run of the Action
		 *
		 * @param action_id - new Action ID
		 */
		virtual void SetActionId(uint16_t action_id) = 0;
	};

	/**
	 * @brief Action Class
	 *
	 * Defines a base class for SDK Actions. Provides basic template for concrete implementations.
	 * Also includes code for Thread sync with client core.
	 * All Actions that can be performed by the Client Core must inherit from this class.
	 * This is a pure virtual class and cannot be instantiated
	 *
	 */
	class Action {
	public:
		/**
		 * Define a type for Create Factory method. Takes Action state as argument and returns a unique_ptr to a new action instance
		 */
		typedef std::function<std::unique_ptr<Action>(std::shared_ptr<ActionState> p_action_state)> CreateHandlerPtr;

		/**
		 * @brief Get Type of this Action
		 * @return ActionType
		 */
		ActionType GetActionType() {
			return action_type_;
		}

		/**
		 * @brief Get information/description about the current action
		 *
		 * Gets runtime information about the currently running Action if it was set when the action was created
		 *
		 * @return String containing Info Text
		 */
		util::String GetActionInfo() {
			return action_info_string_;
		}

		/**
		 * @brief Sets the parent thread sync variable
		 *
		 * @param p_thread_continue - Pointer to the new sync variable to use
		 */
		void SetParentThreadSync(std::shared_ptr<std::atomic_bool> p_thread_continue) {
			p_thread_continue_ = p_thread_continue;
		}

		/**
		 * @brief Virtual base function for Performing Action
		 *
		 * This function is called by Client Core and defines how the Action is Performed.
		 * This is a pure virtual function. Inherited classes MUST implement this.
		 *
		 * @param p_network_connection - Network connection to be used to perform the Action
		 * @param p_action_data - Action data to be used for this run of the action
		 * @return ResponseCode indicating result of the API call
		 */
		virtual ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
										   std::shared_ptr<ActionData> p_action_data) = 0;

		// Rule of 5 stuff
		// Disabling default, move and copy constructors
		// Actions instances can be run as threads if needed and should not be copied or moved
		Action() = delete;                                // Delete Default constructor
		Action(const Action &) = delete;                    // Copy constructor
		Action(Action &&) = delete;                        // Move constructor
		Action& operator=(const Action &) = delete;    // Copy assignment operator
		Action& operator=(Action &&) = delete;        // Move assignment operator
		virtual ~Action() = default;                    // Default destructor

		/**
		 * @brief Action Constructor
		 *
		 * @param action_type - Type fo the Action being instantiated
		 * @param action_info_string - Info string describing the action
		 */
		Action(ActionType action_type, util::String action_info_string);

	protected:
		ActionType action_type_;                                ///< Type of the action
		util::String action_info_string_;                        ///< Info string
		std::shared_ptr<std::atomic_bool> p_thread_continue_;    ///< Shared atomic variable used for sync when action is run in separate thread

		/**
		 * @brief Generic Network Read function for all actions
		 * @param p_network_connection - Network connection to be used to perform Read
		 * @param read_buf - Buffer read data should be copied to. Assumed to already have enough memory reserved
		 * @param bytes_to_read - Number of bytes to read
		 * @return ResponeCode indicating result of the API call
		 */
		ResponseCode ReadFromNetworkBuffer(std::shared_ptr<NetworkConnection> p_network_connection,
										   util::Vector<unsigned char> &read_buf, size_t bytes_to_read);

		/**
		 * @brief Generic Network Write function for all actions
		 * @param p_network_connection - Network connection to be used to perform Write
		 * @param read_buf - Buffer containing data to be written to the network instance
		 * @return ResponeCode indicating result of the API call
		 */
		ResponseCode WriteToNetworkBuffer(std::shared_ptr<NetworkConnection> p_network_connection,
										  const util::String &write_buf);
	};
}
