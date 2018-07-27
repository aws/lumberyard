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
 * @file NetworkConnection.hpp
 * @brief Network interface base class for IoT Client
 *
 * Defines an interface to the Network layer to be used by the MQTT client.
 * Starting point for porting the SDK to the networking layer of a new platform.
 */

#pragma once

#include <cstdint>
#include <string>
#include <mutex>
#include <memory>

#include "util/Core_EXPORTS.hpp"
#include "util/memory/stl/String.hpp"
#include "util/memory/stl/Vector.hpp"

#include "ResponseCode.hpp"

namespace awsiotsdk {
	/**
	 * @brief Network Connection Class
	 *
	 * Defines an interface to the Network layer to be used by the MQTT client.
	 * Starting point for porting the SDK to the networking layer of a new platform.
	 *
	 * This is an abstract class and cannot be instantiated.
	 */
	AWS_API_EXPORT class NetworkConnection {
	protected:
		/**
		 * Both the below mutexes must be locked before connect/disconnect
		 */
		std::mutex read_mutex;        ///< Mutex for synchronizing read operations
		std::mutex write_mutex;        ///< Mutex for synchronizing write operations

		/**
		 * @brief Create a Network socket and open the connection
		 *
		 * Internal implementation of the Connect function to be provided by the derived class
		 *
		 * Creates an open socket connection including Network handshake.
		 *
		 * @return ResponseCode - successful connection or Network error
		 */
		virtual ResponseCode ConnectInternal() = 0;

		/**
		 * @brief Write bytes to the network socket
		 *
		 * Internal implementation of the Write function to be provided by the derived class
		 *
		 * @param util::String - const reference to buffer which should be written to socket
		 * @return size_t - number of bytes written
		 * @return ResponseCode - successful write or Network error code
		 */
		virtual ResponseCode WriteInternal(const util::String &buf, size_t &size_written_bytes_out) = 0;

		/**
		 * @brief Read bytes from the network socket
		 *
		 * Internal implementation of the Read function to be provided by the derived class
		 *
		 * @param util::String - reference to buffer where read bytes should be copied
		 * @param size_t - number of bytes to read
		 * @param size_t - reference to store number of bytes read
		 * @return ResponseCode - successful read or Network error code
		 */
		virtual ResponseCode ReadInternal(util::Vector<unsigned char> &buf, size_t buf_read_offset,
										  size_t size_bytes_to_read, size_t &size_read_bytes_out) = 0;

		/**
		 * @brief Disconnect from network socket
		 *
		 * Internal implementation of the Disconnect function to be provided by the derived class
		 *
		 * @return ResponseCode - successful read or Network error code
		 */
		virtual ResponseCode DisconnectInternal() = 0;

	public:
		/**
		 * @brief Check if Network layer is still connected
		 *
		 * Called to check if the Network layer is still connected or not.
		 *
		 * @return ResponseCode - Network error code indicating status of network physical layer connection
		 */
		virtual bool IsConnected() = 0;

		/**
		 * @brief Check if Network Physical layer is still connected
		 *
		 * Called to check if the Network Physical layer is still connected or not.
		 *
		 * @return bool - indicating status of network physical layer connection
		 */
		virtual bool IsPhysicalLayerConnected() = 0;

		/**
		 * @brief Create a Network socket and open the connection
		 *
		 * Calls the internal connect function after obtaining read and write locks
		 *
		 * @return ResponseCode - successful connection or Network error
		 */
		virtual ResponseCode Connect() final;

		/**
		 * @brief Write bytes to the network socket
		 *
		 * Calls the internal write function after obtaining write lock
		 *
		 * @param util::String - const reference to buffer which should be written to socket
		 * @return size_t - number of bytes written or Network error
		 * @return ResponseCode - successful write or Network error code
		 */
		virtual ResponseCode Write(const util::String &buf, size_t &size_written_bytes_out) final;

		/**
		 * @brief Read bytes from the network socket
		 *
		 * Calls the internal read function after obtaining read lock
		 *
		 * @param util::String - reference to buffer where read bytes should be copied
		 * @param size_t - number of bytes to read
		 * @param size_t - reference to store number of bytes read
		 * @return ResponseCode - successful read or Network error code
		 */
		virtual ResponseCode Read(util::Vector<unsigned char> &buf, size_t buf_read_offset,
								  size_t size_bytes_to_read, size_t &size_read_bytes_out) final;

		/**
		 * @brief Disconnect from network socket
		 *
		 * Calls the internal disconnect function after obtaining read and write locks
		 * This will be called by the SDK for both manual and auto-disconnect.
		 * It separates the Disconnect logic from destroy, Network stack is NOT destroyed by this API
		 * SDK should still be able to reconnect after Disconnect, but not after Destroy
		 *
		 * @return ResponseCode - successful read or Network error code
		 */
		virtual ResponseCode Disconnect() final;

		virtual ~NetworkConnection() {}
	};
}
