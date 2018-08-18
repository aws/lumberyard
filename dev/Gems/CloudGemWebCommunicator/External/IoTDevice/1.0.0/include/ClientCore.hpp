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
 * @file ClientCore.hpp
 * @brief Contains the Client Core class
 *
 * Defines the Client Core class which is a generic Action executor class
 * Client Core instances are responsible for maintaining the Action Registry
 * and running Actions on request. They are also responsible for spawning
 * threads for Actions.
 *
 */

#pragma once

#include "ClientCoreState.hpp"
#include "util/threading/ThreadTask.hpp"

namespace awsiotsdk {

	/**
	 * @brief Client Core Class
	 *
	 * Defining a class for the Core Client.
	 * This class is responsible for performing both Sync and Async actions
	 * It contains the action queue, an action registry and a map to keep track of running threads
	 * It also keeps track of the Common Action State as well as the Network connection
	 *
	 */
	AWS_API_EXPORT class ClientCore {
	protected:
		util::Map<ActionType, std::shared_ptr<util::Threading::ThreadTask>> thread_map_;    ///< Map for storing currently active threads

		std::shared_ptr<ClientCoreState> p_client_core_state_;								///< Client Core state instance

		/**
		 * @brief Constructor
		 *
		 * @param p_network_connection - Network Connection instance to be passed as argument to actions
		 * @param p_state - Client Core state instance
		 */
		ClientCore(std::shared_ptr<NetworkConnection> p_network_connection, std::shared_ptr<ClientCoreState> p_state);

	public:
		// Disabling default, copy and move constructors. Defining the virtual destructor
		// Class contains thread instances. Should not be copied or moved
		ClientCore() = delete;                                    // Delete Default constructor
		ClientCore(const ClientCore &) = delete;                    // Delete Copy constructor
		ClientCore(ClientCore &&) = delete;                        // Delete Move constructor
		ClientCore& operator=(const ClientCore &) = delete;    // Delete Copy assignment operator
		ClientCore& operator=(ClientCore &&) = delete;            // Delete Move assignment operator
		virtual ~ClientCore();                                    // Defined destructor

		/**
		 * @brief Enable/Disable processing of queued actions
		 *
		 * @param process_queued_actions - boolean value indicating new state
		 */
		void SetProcessQueuedActions(bool process_queued_actions) {
			p_client_core_state_->SetProcessQueuedActions(process_queued_actions);
		}

		/**
		 * @brief Factory method for creating a Client Core instance
		 *
		 * @param p_network_connection - Network Connection instance to be passed as argument to actions
		 * @param p_state - Client Core state instance
		 * @return std::unique_ptr<ClientCore> instance
		 */
		static std::unique_ptr<ClientCore>
		Create(std::shared_ptr<NetworkConnection> p_network_connection, std::shared_ptr<ClientCoreState> p_state);

		/**
		 * @brief Register Action for execution by Client Core
		 *
		 * This function allows Actions to be registered to be executed at a later stage by Client Core.
		 * Actions must be registered before PerformAction can be called using the Action Type.
		 * This also applies to Creating Action runners which allow running Actions in dedicated Thread Tasks.
		 * Only one Action can be registered to each Action Type. If a second call is made with the same Action Type,
		 * the previous registration will be overwritten
		 *
		 * @param action_type - Type of the Action that will be creating using the provided handler
		 * @param p_action_create_handler - Factory method pointer which returns an Action instance
		 * @return ResponseCode indicating result of the API call
		 */

		ResponseCode RegisterAction(ActionType action_type, Action::CreateHandlerPtr p_action_create_handler);

		/**
		 * @brief Perform Action in Blocking Mode
		 *
		 * This API will perform the Action in Blocking mode. The timeout for the action to give a valid response
		 * is provided as an argument. This API stops processing of all outbound actions until Response is received
		 * for the requested Action Type
		 *
		 * @param action_type - Type of the Action to be executed. Must be registered
		 * @param action_data - Action Data to be passed as argument to the Action instance
		 * @param action_reponse_timeout - Timeout for this API call
		 * @return ResponseCode indicating result of the API call
		 */
		ResponseCode PerformAction(ActionType action_type, std::shared_ptr<ActionData> action_data,
								   std::chrono::milliseconds action_reponse_timeout);

		/**
		 * @brief Perform Action in Asynchronous Mode
		 *
		 * This API will enqueue the the Action to be performed Asynchronously. If an Async Ack Handler is provided
		 * in the Action data, and the Action expects to receive an Ack, the calling Action will be notified using
		 * Action ID and Response Code. This API returns Action ID as an out parameter
		 *
		 * @param action_type - Type of the Action to be executed. Must be registered
		 * @param action_data - Action Data to be passed as argument to the Action instance
		 * @param [out] action_id_out - Action ID assigned to this request
		 *
		 * @return ResponseCode indicating result of the enqueue operation
		 */
		ResponseCode PerformActionAsync(ActionType action_type, std::shared_ptr<ActionData> action_data,
										uint16_t &action_id_out);

		/**
		 * @brief Create Thread Task to execute request Action Type
		 *
		 * This API will create a new instance of the Action Type that is request in the API call and call perform
		 * action on that instance in a new Thread Task. If the Action is Thread Aware, it will be executed until
		 * it finishes or the Thread Task is terminated (Usually on exit).
		 *
		 * @param action_type - Type of the Action to be executed. Must be registered
		 * @param action_data - Action Data to be passed as argument to the Action instance
		 * @return ResponseCode indicating result of the API call
		 */
		ResponseCode CreateActionRunner(ActionType action_type, std::shared_ptr<ActionData> action_data);
	};
}
