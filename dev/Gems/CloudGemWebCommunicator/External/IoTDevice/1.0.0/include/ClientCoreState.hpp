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
 * @file ClientCoreState.hpp
 * @brief
 *
 */

#pragma once

#include <condition_variable>
#include <chrono>

#include "util/Core_EXPORTS.hpp"

#include "util/Utf8String.hpp"
#include "util/memory/stl/Map.hpp"
#include "util/memory/stl/Queue.hpp"

#include "Action.hpp"
#include "ResponseCode.hpp"
#include "NetworkConnection.hpp"

/**
 * Default sleep duration between each execution of Client Core thread operations
 */
#define DEFAULT_CORE_THREAD_SLEEP_DURATION_MS 100

/**
 * Max size of the queue
 */
#define DEFAULT_MAX_QUEUE_SIZE 16

namespace awsiotsdk {

	/**
	 * @brief Client Core State Class
	 *
	 * Defining a class for the Core Client State.
	 * This class is responsible for maintaing the state information for the core client
	 * It can also be extended to provide state information to Actions.
	 * It contains the action queue, an action registry and also keeps track of
	 * the Common Action State as well as the Network connection
	 *
	 */
	class ClientCoreState : public ActionState {
	protected:

		/**
		 * @brief Pending Ack Data Class
		 *
		 * Defining an internal class for storing information about Pending Acks.
		 *
		 */
		class PendingAckData {
		public:
			std::chrono::system_clock::time_point time_of_request_;            ///< Time at which the request was sent
			ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler_;    ///< Handler to which response must be sent
		};

		std::atomic<uint16_t> next_action_id_;                    ///< Atomic, ID of the next Action that will be enqueued
		std::atomic_int cur_core_threads_;                        ///< Atomic, Count of currently running core threads
		std::atomic_int max_hardware_threads_;                    ///< Atomic, Count of the maximum allowed hardware threads
		std::atomic_size_t max_queue_size_;                        ///< Atomic, Current configured max queue size
		std::chrono::seconds ack_timeout_;                        ///< Timeout for pending Acks, older Acks are deleted with a failed response

		std::mutex register_action_lock_;                    ///< Mutex for Register Action Request flow
		std::mutex ack_map_lock_;                    ///< Mutex for Ack Map operations

		// Used to perform blocking sync actions
		std::mutex sync_action_request_lock_;                    ///< Mutex for Sync Action Request flow
		std::mutex sync_action_response_lock_;                    ///< Mutex for Sync Action Response flow
		std::condition_variable sync_action_response_wait_;        ///< Condition variable used to wake up calling thread on Sync Action response
		ResponseCode sync_action_response_;                        ///< Variable to store received Sync Action response

		std::atomic_bool process_queued_actions_;                ///< Atomic, indicates whether currently queued Actions should be processed or not
		std::shared_ptr<std::atomic_bool> continue_execution_;    ///< Atomic, Used to synchronize running threads, false value causes running threads to stop

		util::Map<ActionType, std::unique_ptr<Action>> action_map_;                    ///< Map containing currently initialized Action Instances
		util::Map<uint16_t, std::unique_ptr<PendingAckData>> pending_ack_map_;        ///< Map containing currently pending Acks
		util::Map<ActionType, Action::CreateHandlerPtr> action_create_handler_map_;    ///< Map containing currently registered Action Types and corrosponding Factories

		util::Queue<std::pair<ActionType, std::shared_ptr<ActionData>>> outbound_action_queue_;    ///< Queue of outbound actions

		/**
		 * @brief Internal Action Handler for Sync Action responses
		 *
		 * @param action_id - ID of the Action that response was received for
		 * @param rc - Received response
		 */
		void SyncActionHandler(uint16_t action_id, ResponseCode rc);

	public:
		/**
		 * @brief Network connection instance to use for this instance of the Client
		 * This is shared between Actions, public to avoid multiple shared pointer operations while passing as argument by ClientCore
		 */
		std::shared_ptr<NetworkConnection> p_network_connection_;

		/**
		 * @brief Overload for Get next Action ID
		 * @return uint16_t Action ID
		 */
		virtual uint16_t GetNextActionId() {
			if(UINT16_MAX == next_action_id_) {
				next_action_id_ = 1;
				return UINT16_MAX;
			}
			return next_action_id_++;
		};

		/**
		 * @brief Get current value of maximum action queue size
		 * @return size_t max_queue_size_
		 */
		size_t GetMaxActionQueueSize() { return max_queue_size_; }

		/**
		 * @brief Set max size for action queue
		 * @param size_t max_queue_size
		 */
		void SetMaxActionQueueSize(size_t max_queue_size) { max_queue_size_ = max_queue_size; }

		/**
		 * @brief Get pointer to sync point used for execution status of the Core instance
		 *
		 * This sync point is used to indicate SDK is still continuing execution. Set to false when exiting
		 *
		 * @return std::shared_ptr<std::atomic_bool> pointer to the sync point
		 */
		std::shared_ptr<std::atomic_bool> GetCoreExecutionSyncPoint() { return continue_execution_; }

		/**
		 * @brief Sets whether the Client is allowed to process queue actions
		 * @param process_queued_actions value to set it to
		 */
		void SetProcessQueuedActions(bool process_queued_actions) { process_queued_actions_ = process_queued_actions; }

		/**
		 * @brief Get whether the Client can process queued actions
		 * @return boolean value indicating status
		 */
		bool CanProcessQueuedActions() { return process_queued_actions_; }

		/**
		 * @brief Process the outbound action queue
		 *
		 * This function processes the actions queued up in the Outbound action queue.
		 * The function accepts a Sync point that can be used to control execution in a separate thread.
		 * If the value is set to false for the sync point, the function will perform one action from the queue.
		 * This puts the running thread to sleep if there are no queued up actions.
		 * DO NOT call from main thread unless you have a separate thread to queue up actions
		 *
		 * @param thread_task_out_sync
		 */
		void ProcessOutboundActionQueue(std::shared_ptr<std::atomic_bool> thread_task_out_sync);

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
		 * @brief Register Action for execution by Client Core
		 *
		 * This function allows Actions to be registered to be executed at a later stage by Client Core.
		 * Actions must be registered before PerformAction can be called using the Action Type.
		 * This also applies to Creating Action runners which allow running Actions in dedicated Thread Tasks.
		 * Only one Action can be registered to each Action Type. If a second call is made with the same Action Type,
		 * the previous registration will be overwritten
		 *
		 * @param action_type - Type of the Action that will be created using the provided handler
		 * @param p_action_create_handler - Factory method pointer which returns an Action instance
		 * @param p_action_state - Shared_ptr to use as argument for Action create
		 * @return ResponseCode indicating result of the API call
		 */
		ResponseCode RegisterAction(ActionType action_type, Action::CreateHandlerPtr p_action_create_handler,
									std::shared_ptr<ActionState> p_action_state);

		/**
		 * @brief Get the Create Factory Method for the specified action type
		 *
		 * @param action_type - Type of the Action for which the handler is required
		 * @param p_action_create_handler[out] - Create factory method for this Action
		 * @return ResponseCode indicating result of the API call
		 */
		ResponseCode GetActionCreateHandler(ActionType action_type, Action::CreateHandlerPtr *p_action_create_handler);

		/**
		 * @brief Enqueue Action for processing in Outbound Queue
		 *
		 * @param action_type - Type of the Action
		 * @param action_data - Data to be passed to perform Action
		 * @param action_id_out[out] - Action ID that was assigned to this action by the Client
		 * @return ResponseCode indicating result of the API call
		 */
		ResponseCode EnqueueOutboundAction(ActionType action_type, std::shared_ptr<ActionData> action_data,
										   uint16_t &action_id_out);

		/**
		 * @brief Register Ack Handler for provided action id
		 * @param action_id - Action ID
		 * @param p_async_ack_handler - Handler to call on response
		 * @return ResponseCode indicating result of the API call
		 */
		ResponseCode RegisterPendingAck(uint16_t action_id,
										ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler);

		/**
		 * @brief Delete Ack Handler for specified Action ID
		 * @param action_id - Action ID
		 */
		void DeletePendingAck(uint16_t action_id);

		/**
		 * @brief Call registered Ack handler if it exists for specified Packet id
		 * @param action_id - Action ID
		 * @param rc - Response Code to pass to the Handler if found
		 */
		void ForwardReceivedAck(uint16_t action_id, ResponseCode rc);

		/**
		 * @brief Delete all expired Acks
		 *
		 * Deletes all Acks where the timeouts have expired. Responds with Code indicating request timeout
		 */
		void DeleteExpiredAcks();

		/**
		 * @brief Default Constructor
		 */
		ClientCoreState();

		/**
		 * @brief Destructor
		 */
		virtual ~ClientCoreState();

		// Rule of 5 stuff
		// Contains data for running thread tasks, should not be moved or copied
		ClientCoreState(const ClientCoreState &) = delete;                    // Copy constructor
		ClientCoreState(ClientCoreState &&) = delete;                        // Move constructor
		ClientCoreState &operator=(const ClientCoreState &) = delete;        // Copy assignment operator
		ClientCoreState &operator=(ClientCoreState &&) = delete;            // Move assignment operator
	};
}

