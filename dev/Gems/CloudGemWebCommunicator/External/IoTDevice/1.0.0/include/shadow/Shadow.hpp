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
 *
 */

/**
 * @file Shadow.hpp
 * @brief This file defines a shadow type for AWS IoT Shadow operations
 *
 * This file defines a shadow type for AWS IoT Shadow operations. It also defines
 * related types ShadowRequestType, ShadowResponseType and a shadow response handler
 */


#pragma once

#include <memory>
#include <chrono>
#include <mutex>
#include <atomic>

#include "util/memory/stl/String.hpp"
#include "util/memory/stl/Vector.hpp"
#include "util/JsonParser.hpp"

#include "mqtt/Client.hpp"

namespace awsiotsdk {
	/**
	 * @brief Define a type for Shadow Requests
	 *
	 * Documents type is not currently supported
	 */
	enum class ShadowRequestType {
		Get = 0x01,
		Update = 0x02,
		Delete = 0x04,
		Delta = 0x08
	};

	/**
	 * @brief Define a type for Shadow Responses
	 */
	enum class ShadowResponseType {
		Rejected = 0,
		Accepted = 1,
		Delta = 2
	};

	/**
	 * @brief Define a type for Shadow
	 */
	class Shadow {
	public:
		/**
		 * @brief Request Handler type for Shadow requests. Called after Shadow instance proccesses incoming message
		 *
		 * Takes the following as parameters
		 * util::String - Thing Name for which response was received
		 * ShadowRequestType - Request Type on which response was received
		 * ShadowResponseType - Response Type
		 * util::JsonDocument - JsonPayload of the response
		 */
		typedef std::function<ResponseCode(util::String, ShadowRequestType, ShadowResponseType,
										   util::JsonDocument &)> RequestHandlerPtr;

		/**
		 * @brief Constructor
		 *
		 * @param p_mqtt_client - MQTT Client instance used for this Shadow, can NOT be changed later
		 * @param mqtt_command_timeout - Timeout to use for MQTT Commands
		 * @param thing_name - Thing name for this shadow
		 * @param client_token_prefix - Client Token prefix to use for shadow operations
		 */
		Shadow(std::shared_ptr<MqttClient> p_mqtt_client, std::chrono::milliseconds mqtt_command_timeout,
			   util::String &thing_name, util::String &client_token_prefix);

		// Rule of 5 stuff
		// Disable copying/moving because subscription handler callbacks will not carry over automatically
		Shadow() = delete;								// Default constructor
		Shadow(const Shadow&) = delete;					// Delete Copy constructor
		Shadow(Shadow&&) = delete;						// Delete Move constructor
		Shadow& operator=(const Shadow&) & = delete;	// Delete Copy assignment operator
		Shadow& operator=(Shadow&&) & = delete;			// Delete Move assignment operator
		virtual ~Shadow();								// Custom destructor

		/**
		 * @brief Factory method to create Shadow instances
		 *
		 * @param p_mqtt_client - MQTT Client instance used for this Shadow, can NOT be changed later
		 * @param mqtt_command_timeout - Timeout to use for MQTT Commands
		 * @param thing_name - Thing name for this shadow
		 * @param client_token_prefix - Client Token prefix to use for shadow operations
		 *
		 * @return std::unique_ptr to a Shadow instance
		 */
		static std::unique_ptr<Shadow> Create(std::shared_ptr<MqttClient> p_mqtt_client,
											  std::chrono::milliseconds mqtt_command_timeout, util::String &thing_name,
											  util::String &client_token_prefix);

		/**
		 * @brief Update device shadow
		 *
		 * This function can be used to update a device shadow. The document passed here must have the same
		 * structure as a device shadow document. Whatever is provided here will be merged into the device shadow json
		 * with one of the below options:
		 * 1) Key exists in both current state and the provided document : Provided document value is used
		 * 2) Key exists only in current state : No changes, kept as is
		 * 3) Key exists only in the provided document : New document's key and value is copied to device shadow json
		 *
		 * To easily generate a source document, please use either the GetEmptyShadowDocument function to get an empty
		 * document or get either the current device state or server state document using the corrosponding functions
		 *
		 * @note Shadow document structure can be seen here - http://docs.aws.amazon.com/iot/latest/developerguide/thing-shadow-document.html
		 *
		 * @param document - JsonDocument containing the new updates
		 *
		 * @return ResponseCode indicating status of request
		 */
		ResponseCode UpdateDeviceShadow(util::JsonDocument &document);

		/**
		 * @brief Get reported state of the shadow on the device
		 * @return JsonDocument containing a copy of the reported state of shadow that exists on the device
		 */
		util::JsonDocument GetDeviceReported();

		/**
		 * @brief Get desired state of the shadow on the device
		 * @return JsonDocument containing a copy of the desired state of shadow that exists on the device
		 */
		util::JsonDocument GetDeviceDesired();

		/**
		 * @brief Get state document of the shadow on the device
		 * @return JsonDocument containing a copy of the shadow document that exists on the device
		 */
		util::JsonDocument GetDeviceDocument();

		/**
		 * @brief Get reported state of the shadow state received from the server
		 * @return JsonDocument containing a copy of the reported state that was last received from the server
		 */
		util::JsonDocument GetServerReported();

		/**
		 * @brief Get desired state of the shadow state received from the server
		 * @return JsonDocument containing a copy of the desired state that was last received from the server
		 */
		util::JsonDocument GetServerDesired();

		/**
		 * @brief Get state document of the shadow state received from the server
		 * @return JsonDocument containing a copy of the shadow document that was last received from the server
		 */
		util::JsonDocument GetServerDocument();

		/**
		 * @brief Perform a Get operation for this shadow
		 *
		 * If Accepted, this clears the current server shadow state and updates it with the new state received
		 * from the server.  If the subscription for this request type doesn't exist, it will also subscribe to the
		 * Accepted and Rejected topics.
		 *
		 * @return ResponseCode indicating status of the request
		 */
		ResponseCode PerformGetAsync();

		/**
		 * @brief Perform an Update operation for this shadow
		 *
		 * This function generates a diff between the current state of the shadow on the device and the last reported
		 * state of the shadow on the server. Then it calls update using this diff. If the subscription for this request
		 * type doesn't exist, it will also subscribe to the Accepted and Rejected topics. This does NOT automatically
		 * subscribe to the delta topic.
		 *
		 * @return ResponseCode indicating status of the request
		 */
		ResponseCode PerformUpdateAsync();

		/**
		 * @brief Perform a Delete operation for this shadow
		 *
		 * If accepted, this will delete the shadow from the server. It also clears the shadow state received from the
		 * server and sets it to an empty object. This will NOT affect the shadow state for the device. To do that,
		 * use the UpdateDeviceShadow function with an empty document. If the subscription for this request type doesn't
		 * exist, it will also subscribe to the Accepted and Rejected topics.
		 *
		 * @return ResponseCode indicating status of the request
		 */
		ResponseCode PerformDeleteAsync();

		/**
		 * @brief Handle response for Get Request
		 * @param response_type - Response Type received from the server
		 * @param payload - Json payload received with the response
		 * @return ResponseCode indicating the status of the request
		 */
		ResponseCode HandleGetResponse(ShadowResponseType response_type, util::JsonDocument &payload);

		/**
		 * @brief Handle response for Update Request
		 * @param response_type - Response Type received from the server
		 * @param payload - Json payload received with the response
		 * @return ResponseCode indicating the status of the request
		 */
		ResponseCode HandleUpdateResponse(ShadowResponseType response_type, util::JsonDocument &payload);

		/**
		 * @brief Handle response for Delete Request
		 * @param response_type - Response Type received from the server
		 * @param payload - Json payload received with the response
		 * @return ResponseCode indicating the status of the request
		 */
		ResponseCode HandleDeleteResponse(ShadowResponseType response_type, util::JsonDocument &payload);

		/**
		 * @brief Reset the timestamp generated client suffix
		 */
		void ResetClientTokenSuffix();

		/**
		 * @brief Get the current version number of the shadow
		 * @return uint32_t containing the current shadow version received from the server
		 */
		uint32_t GetCurrentVersionNumber();

		/**
		 * @brief Get whether the server shadow state is in sync
		 *
		 * @return boolean indicating sync status
		 */
		bool IsInSync();

		/**
		 * @brief Add a specific shadow subscription
		 *
		 * This function can be used to add a subscription to various shadow actions. Each action can optionally be
		 * assigned a handler that can process any response that is received. The internal shadow state will be
		 * updated before the response handler is called if it is provided.
		 *
		 * @param request_mapping Mapping of request type to response handler
		 * @return ResponseCode indicating status of operation
		 */
		ResponseCode AddShadowSubscription(util::Map<ShadowRequestType, RequestHandlerPtr> &request_mapping);

		/**
		 * @brief Subscription handler for Shadow actions
		 *
		 * This function is the subscription handler used by this shadow instance for internal action topics.
		 * This function is for internal use ONLY. It is public because the mqtt client needs to be provided with a
		 * reference to this function.
		 *
		 * @param topic_name - Topic name for which publish message is received
		 * @param payload - Payload that was received
		 * @param p_app_handler_data - Context data
		 *
		 * @return ResponseCode indicating status of operation
		 */
		ResponseCode SubscriptionHandler(util::String topic_name, util::String payload,
										 std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);

		/**
		 * @brief Static function that creates and returns an empty Shadow json document
		 * @return util::JsonDocument initialized as empty shadow json document
		 */
		static util::JsonDocument GetEmptyShadowDocument();

	protected:
		bool is_get_subscription_active_;				///< Status of the get subscription
		bool is_update_subscription_active_;			///< Status of the update subscription
		bool is_delete_subscription_active_;			///< Status of the delete subscription
		bool is_delta_subscription_active_;				///< Status of the delta subscription

		uint32_t cur_shadow_version_;					///< Current version of the shadow as received from the server

		util::String thing_name_;						///< Thing name for this shadow instance
		util::String client_token_prefix_;				///< Client token prefix being used for shadow actions
		util::String client_token_;						///< Full client token as generated while constructing this instance
		util::String shadow_topic_action_prefix_;		///< Shadow topic action prefix
		util::String shadow_topic_get_;					///< Get topic for this shadow
		util::String shadow_topic_delta_;				///< Delta topic for this shadow
		util::String shadow_topic_update_;				///< Update topic for this shadow
		util::String shadow_topic_delete_;				///< Delete topic for this shadow
		util::String response_type_delta_text_;			///< Delta response postfix
		util::String response_type_rejected_text_;		///< Rejected reponse postfix
		util::String response_type_accepted_text_;		///< Accepted response postfix

		std::chrono::milliseconds mqtt_command_timeout_;	///< Mqtt command timeout

		util::JsonDocument cur_server_state_document_;	///< Last received shadow state document from the server
		util::JsonDocument cur_device_state_document_;	///< Current shadow state document on the device

		util::Map<ShadowRequestType, RequestHandlerPtr> request_mapping_;	///< Request mappings for shadow actions

		std::shared_ptr<MqttClient> p_mqtt_client_;		///< IoT Client being used by this Shadow instance
	};
}

