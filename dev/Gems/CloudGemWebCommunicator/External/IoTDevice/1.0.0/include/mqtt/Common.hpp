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
 * @file Common.hpp
 * @brief Common class definitions for the MQTT Client
 *
 */

#pragma once

#include "util/Utf8String.hpp"
#include "ResponseCode.hpp"

#define MAX_TOPICS_IN_ONE_SUBSCRIBE_PACKET 8

namespace awsiotsdk {
	namespace mqtt {
		/**
		 * @brief MQTT Version Type
		 *
		 * Defining an MQTT version type. Only 3.1.1 is supported at this time
		 *
		 */
		enum class Version {
			MQTT_3_1_1 = 4    ///< MQTT 3.1.1 (protocol message byte = 4)
		};

		/**
		 * @brief Quality of Service (QoS) Type
		 *
		 * Defining a Quality of Service Type. QoS2 is not supported at this time
		 *
		 */
		enum class QoS {
			QOS0 = 0,	///< QoS0
			QOS1 = 1	///< QoS1
		};

		/**
		 * @brief MQTT Message Types Definition
		 *
		 * Defining a type for MQTT Messages
		 *
		 */
		enum class MessageTypes {
			INVALID = 0,	///< Invalid Message
			CONNECT = 1,	///< CONNECT
			CONNACK = 2,	///< CONNACK
			PUBLISH = 3,	///< PUBLISH
			PUBACK = 4,		///< PUBACK
			PUBREC = 5,		///< PUBREC
			PUBREL = 6,		///< PUBREL
			PUBCOMP = 7,	///< PUBCOMP
			SUBSCRIBE = 8,	///< SUBSCRIBE
			SUBACK = 9,		///< SUBACK
			UNSUBSCRIBE = 10,	///< UNSUBSCRIBE
			UNSUBACK = 11,	///< UNSUBACK
			PINGREQ = 12,	///< PINGREQ
			PINGRESP = 13,	///< PINGRESP
			DISCONNECT = 14,	///< DISCONNECT
			RESERVED = 15	///< Reserved for future use
		};

		/**
		 * @brief Last Will and Testament Definition
		 *
		 * Defining a type for the MQTT "Last Will and Testament" (LWT) parameters.
		 * @note Retained messages are \b NOT supported by the AWS IoT Service at the time of this SDK release.
		 *
		 */
		class WillOptions {
		private:
			std::unique_ptr<Utf8String> p_struct_id_;	///< The eyecatcher for this structure.  must be MQTW
			bool is_retained_;							///< NOT supported. The retained flag for the LWT message
			QoS qos_;									///< QoS of LWT message
			std::unique_ptr<Utf8String> p_topic_name_;	///< The LWT topic to which the LWT message will be published
			util::String message_;						///< Message to be delivered as LWT

		public:
			/**
			 * @brief Constructor
			 *
			 * @param is_retained - MQTT Is Retained flag
			 * @param qos - QoS for the Will Message
			 * @param p_topic_name - Topic name on which to publish the will message
			 * @param p_message - Payload of the message to publish
			 */
			WillOptions(bool is_retained, QoS qos, std::unique_ptr<Utf8String> p_topic_name, util::String &message);

			// Disabling default constructor. Defining a virtual destructor
			// Ensure WillOptions Instances can be copied/moved
			WillOptions() = delete;									// Delete Default constructor
            // LY 2013 Fix
            // LY 2013 Fix
			virtual ~WillOptions() { }

			// Define Copy constructor
			WillOptions(const WillOptions &source);

			// Define Copy assignment operator overload  
            // LY 2013 Fix
			WillOptions& operator=(const WillOptions &source) {
				is_retained_ = source.is_retained_;
				qos_ = source.qos_;
				p_struct_id_ = Utf8String::Create(source.p_struct_id_->ToStdString());
				p_topic_name_ = Utf8String::Create(source.p_topic_name_->ToStdString());
				message_ = source.message_;

				return *this;
			}

			/**
			 * @brief Factory method to Create a Will Options instance
			 *
			 * @param is_retained - MQTT Is Retained flag
			 * @param qos - QoS for the Will Message
			 * @param p_topic_name - Topic name on which to publish the will message
			 * @param p_message - Payload of the message to publish
			 *
			 * @return Returns a unique pointer to a WillOptions Instance
			 */
			static std::unique_ptr<WillOptions> Create(bool is_retained, QoS qos,
													   std::unique_ptr<Utf8String> p_topic_name, util::String &message);

			/**
			 * @brief Get length of Will Options message
			 *
			 * @return size_t Length
			 */
			size_t Length();

			/**
			 * @brief Serialize and write Will Options to provided buffer
			 *
			 * @param buf - Reference to target buffer
			 */
			void WriteToBuffer(util::String &buf);

			/**
			 * @brief Set Connect flags in the provided buffer based on Will Options instance
			 *
			 * @param p_flag Target buffer
			 */
			void SetConnectFlags(unsigned char &p_flag);
		};

		/**
		 * @brief MQTT Subscription Handler Context Data
		 *
		 * This class can be used to provide customer context data to be provided with each Subscription Handler call.
		 * Uses a pure virtual destructor to allow for polymorphism
		 */
		class SubscriptionHandlerContextData {
		public:
			virtual ~SubscriptionHandlerContextData() = 0;
		};

		/**
		 * @brief MQTT Subscription Definition
		 *
		 * Defining a type for the MQTT Subscriptions
		 * Contains all information required to process a subscription including callback handler
		 *
		 * @note Also defines a type for the Application callback handler - Subscription::ApplicationCallbackHandlerPtr
		 *
		 */
		class Subscription {
		public:
			/**
			 * @brief Define handler for Application Callbacks.
			 *
			 * This handler is used to provide notification to the application when a message is received on
			 * a subscribed topic
			 */
			typedef std::function<ResponseCode(util::String topic_name, util::String payload, std::shared_ptr<SubscriptionHandlerContextData> p_app_handler_data)> ApplicationCallbackHandlerPtr;

			ApplicationCallbackHandlerPtr p_app_handler_;	///< Pointer to the Application Handler
			std::shared_ptr<SubscriptionHandlerContextData> p_app_handler_data_;				///< Data to be passed to the Application Handler

			// Disabling default constructor. Defining a virtual destructor
			// Ensure Subscription Instances can be copied/moved
			Subscription() = delete;									// Delete Default constructor
			Subscription(const Subscription &) = default;				// Delete Copy constructor
            // LY 2013 Fix
			Subscription& operator=(const Subscription &) = default;	// Delete Copy assignment operator
            // LY 2013 Fix
			virtual ~Subscription() {
				// Do NOT delete App handler data
			}

			/**
			 * @brief Constructor
			 *
			 * @param p_topic_name - Topic name for this subscription
			 * @param max_qos - Max QoS
			 * @param p_app_handler - Application Handler instance
			 * @param p_app_handler_data - Data to be passed to application handler. Can be nullptr
			 */
			Subscription(std::unique_ptr<Utf8String> p_topic_name, QoS max_qos, ApplicationCallbackHandlerPtr p_app_handler, std::shared_ptr<SubscriptionHandlerContextData> p_app_handler_data);

			/**
			 * @brief Factory method to create a Subscription instance
			 *
			 * @param p_topic_name - Topic name for this subscription
			 * @param max_qos - Max QoS
			 * @param p_app_handler - Application Handler instance
			 * @param p_app_handler_data - Data to be passed to application handler. Can be nullptr
			 *
			 * @return shared_ptr Subscription instance
			 */
			static std::shared_ptr<Subscription> Create(std::unique_ptr<Utf8String> p_topic_name, QoS max_qos, ApplicationCallbackHandlerPtr p_app_handler, std::shared_ptr<SubscriptionHandlerContextData> p_app_handler_data);

			/**
			 * @brief Is Subscription Active?
			 *
			 * @return boolean indicating whether the subscription is active
			 */
			bool IsActive() { return is_active_; }

			/**
			 * @brief Set Subscription status
			 *
			 * @param value - boolean value indicating target status
			 */
			void SetActive(bool value) { is_active_ = value; }

			/**
			 * @brief Get Packet ID for this subscription's Subscribe request
			 *
			 * @return uint16_t ID of the packet
			 */
			uint16_t GetPacketId() { return packet_id_; }

			/**
			 * @brief Set expected index of Ack for this Subscription in the SUBACK packet
			 *
			 * @param packet_id - Expected packet id
			 * @param index_in_packet - Expected Index in packet
			 */
			void SetAckIndex(uint16_t packet_id, uint8_t index_in_packet) { packet_id_ = packet_id; index_in_packet_ = index_in_packet; }

			/**
			 * @brief Get Max QoS for this subscription
			 * @return QoS value
			 */
			QoS GetMaxQos() { return max_qos_; }

			/**
			 * @brief Set Max QoS for this subscription
			 * @param max_qos Target QoS value
			 */
			void SetMaxQos(mqtt::QoS max_qos) { max_qos_ = max_qos; }

			/**
			 * @brief Get Length of topic name for this subscription
			 * @return size_t Length
			 */
			size_t GetTopicNameLength() { return p_topic_name_->Length(); }

			/**
			 * @brief Get Topic Name
			 * @return shared_ptr to a Utf8String containing topic name
			 */
			std::shared_ptr<Utf8String> GetTopicName() { return p_topic_name_; }

			/**
			 * @brief Is this subscription in the Suback with given packet ID and index
			 * @param packet_id - Packet ID of received SUBACK
			 * @param index_in_packet - Index in SUBACK
			 * @return boolean indicating whether this Subscription was the target for the received SUBACK
			 */
			bool IsInSuback(uint16_t packet_id, uint8_t index_in_packet) { return (packet_id == packet_id_ && index_in_packet == index_in_packet_); }
		protected:
			bool is_active_;							///< Boolean indicating weather the subscription is active or not
			uint16_t packet_id_;						///< Packet Id of the Subscribe/Unsubscribe Packet
			uint8_t index_in_packet_;					///< Index of the subscription in the Subscribe/Unsubscribe Packet
			QoS max_qos_;								///< Max QoS for messages on this subscription
			std::shared_ptr<Utf8String> p_topic_name_;	///< Topic Name for this subscription
		};
	}
}
