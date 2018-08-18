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
 * @file Packet.hpp
 * @brief
 */

#pragma once

#include <string>
#include <iostream>
#include <memory>

#include "util/Utf8String.hpp"

#include "Action.hpp"
#include "ResponseCode.hpp"
#include "NetworkConnection.hpp"

#include "mqtt/Common.hpp"

namespace awsiotsdk {
	namespace mqtt {
		/**
		 * @brief Define a class for the MQTT Fixed header
		 */
		class PacketFixedHeader {
		private:
			MessageTypes message_type_;			///< Message type for this Fixed Header instance
			unsigned char fixed_header_byte_;	///< Unsigned char byte to store in required formate
			size_t remaining_length_;			///< Remaining Length
			bool is_valid_;						///< Is this header valid/has been initialized?

		public:
			/**
			 * @brief Constructor
			 * @return Initializes the fixed header with default values, sets is_valid_ to false
			 */
			PacketFixedHeader();

			/**
			 * @brief Initialize the fixed header with provided values. Checks for validity
			 *
			 * @param message_type MQTT message type
			 * @param is_duplicate Is this a duplicate message (For publish messages)
			 * @param qos QoS to use for this message (For publish messages)
			 * @param is_retained MQTT is retained flag (For publish messages)
			 * @param rem_len Remaining length
			 *
			 * @return ResponseCode indicating status of request
			 */
			ResponseCode Initialize(MessageTypes message_type, bool is_duplicate, QoS qos, bool is_retained,
									size_t rem_len);

			/**
			 * @brief Is this a valid fixed header?
			 *
			 * @return boolean indicating validity
			 */
			bool isHeaderValid();

			/**
			 * @brief Get message type
			 *
			 * @return Message type
			 */
			MessageTypes GetMessageType();

			/**
			 * @brief Get remaining length
			 * @return Remaining length
			 */
			size_t GetRemainingLength() { return remaining_length_; }

			/**
			 * @brief Get number of bytes required to store remaining length
			 * @return Number of butes required to store remaining length
			 */
			size_t GetRemainingLengthByteCount();

			/**
			 * @brief Get length of the fixed header
			 * @return Length
			 */
			size_t Length() {
				return GetRemainingLengthByteCount() + 1;
			}

			/**
			 * @brief Append this header to a string
			 *
			 * @param p_buf Reference to target string
			 */
			void AppendToBuffer(util::String &p_buf);
		};

		/**
		 * @brief Define a base class for all MQTT Packet types
		 */
		class Packet : public ActionData {
		protected:
			PacketFixedHeader fixed_header_;		///< Fixed header for this packet instance
			size_t packet_size_;					///< Size of the packet
			size_t serialized_packet_length_;		///< Serialized length of the entire packet including fixed header
			std::atomic_uint_fast16_t packet_id_;	///< Message sequence identifier.  Handled automatically by the MQTT client

		public:
			uint16_t GetActionId() { return packet_id_; }
			void SetActionId(uint16_t action_id) { packet_id_ = action_id; }
			bool isPacketDataValid();

			uint16_t GetPacketId() { return packet_id_; }
			void SetPacketId(uint16_t packet_id) { packet_id_ = packet_id; }

			size_t Size() { return serialized_packet_length_; }

			static void AppendUInt16ToBuffer(util::String &buf, uint16_t value);
			static void AppendUtf8StringToBuffer(util::String &buf, std::unique_ptr<Utf8String> &utf8_str);
			static void AppendUtf8StringToBuffer(util::String &buf, std::shared_ptr<Utf8String> &utf8_str);

			static uint16_t ReadUInt16FromBuffer(const util::Vector<unsigned char> &buf, size_t &extract_index);
			static std::unique_ptr<Utf8String> ReadUtf8StringFromBuffer(const util::Vector<unsigned char> &buf, size_t &extract_index);

			virtual util::String ToString() = 0;
			virtual ~Packet() { }
		};
	}
}
