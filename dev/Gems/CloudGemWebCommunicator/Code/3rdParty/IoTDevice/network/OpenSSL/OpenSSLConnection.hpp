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
 * @file OpenSSLConnection.hpp
 * @brief Defines a reference implementation for an OpenSSL library wrapper
 */

#pragma once

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32")
#else
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <string.h>

#include "NetworkConnection.hpp"
#include "ResponseCode.hpp"

namespace awsiotsdk {
	namespace network {
		/**
		 * @brief OpenSSL Wrapper Class
		 *
		 * Defines a reference wrapper for OpenSSL libraries
		 */
		class OpenSSLConnection : public NetworkConnection {
		protected:
			util::String root_ca_location_;				///< Pointer to string containing the filename (including path) of the root CA file.
			util::String device_cert_location_;			///< Pointer to string containing the filename (including path) of the device certificate.
			util::String device_private_key_location_;	///< Pointer to string containing the filename (including path) of the device private key file.
			bool server_verification_flag_;				///< Boolean.  True = perform server certificate hostname validation.  False = skip validation \b NOT recommended.
			bool is_connected_;							///< Boolean indicating connection status
			struct timeval tls_handshake_timeout_;		///< Timeout for TLS handshake command
			struct timeval tls_read_timeout_;			///< Timeout for the TLS Read command
			struct timeval tls_write_timeout_;			///< Timeout for the TLS Write command

			// Endpoint information
			uint16_t endpoint_port_;					///< Endpoint port
			util::String endpoint_;						///< Endpoint for this connection

			SSL_CTX *p_ssl_context_;					///< SSL Context instance
			SSL *p_ssl_handle_;							///< SSL Handle
			int server_tcp_socket_fd_;					///< Server Socket descriptor


			/**
			 * @brief Set TLS socket to non-blocking mode
			 *
			 * @return ResponseCode - successful operation or TLS error
			 */
			ResponseCode SetSocketToNonBlocking();

			/**
			 * @brief Create a TCP socket and open the connection
			 *
			 * Creates an open socket connection
			 *
			 * @return ResponseCode - successful connection or TCP error
			 */
			ResponseCode ConnectTCPSocket();

			/**
			 * @brief Attempt connection
			 *
			 * Attempts TLS Connection
			 *
			 * @return ResponseCode - successful connection or TLS error
			 */
			ResponseCode AttemptConnect();

			/**
			 * @brief Create a TLS socket and open the connection
			 *
			 * Creates an open socket connection including TLS handshake.
			 *
			 * @return ResponseCode - successful connection or TLS error
			 */
			ResponseCode ConnectInternal();

			/**
			 * @brief Write bytes to the network socket
			 *
			 * @param util::String - const reference to buffer which should be written to socket
			 * @return size_t - number of bytes written or Network error
			 * @return ResponseCode - successful write or Network error code
			 */
			ResponseCode WriteInternal(const util::String &buf, size_t &size_written_bytes_out);

			/**
			 * @brief Read bytes from the network socket
			 *
			 * @param util::String - reference to buffer where read bytes should be copied
			 * @param size_t - number of bytes to read
			 * @param size_t - reference to store number of bytes read
			 * @return ResponseCode - successful read or TLS error code
			 */
			ResponseCode ReadInternal(util::Vector<unsigned char> &buf, size_t buf_read_offset,
									  size_t size_bytes_to_read, size_t &size_read_bytes_out);

			/**
			 * @brief Disconnect from network socket
			 *
			 * @return ResponseCode - successful read or TLS error code
			 */
			ResponseCode DisconnectInternal();

		public:

			OpenSSLConnection(util::String endpoint, uint16_t endpoint_port,
							  std::chrono::milliseconds tls_handshake_timeout,
							  std::chrono::milliseconds tls_read_timeout,
							  std::chrono::milliseconds tls_write_timeout,
							  bool server_verification_flag);

			/**
			 * @brief Constructor for the OpenSSL TLS implementation
			 *
			 * Performs any initialization required by the TLS layer.
			 *
			 * @param util::String endpoint - The target endpoint to connect to
			 * @param uint16_t endpoint_port - The port on the target to connect to
			 * @param util::String root_ca_location - Path of the location of the Root CA
			 * @param util::String device_cert_location - Path to the location of the Device Cert
			 * @param util::String device_private_key_location - Path to the location of the device private key file
			 * @param std::chrono::milliseconds tls_handshake_timeout - The value to use for timeout of handshake operation
			 * @param std::chrono::milliseconds tls_read_timeout - The value to use for timeout of read operation
			 * @param std::chrono::milliseconds tls_write_timeout - The value to use for timeout of write operation
			 * @param bool server_verification_flag - used to decide whether server verification is needed or not
			 *
			 */
			OpenSSLConnection(util::String endpoint, uint16_t endpoint_port, util::String root_ca_location,
							  util::String device_cert_location, util::String device_private_key_location,
							  std::chrono::milliseconds tls_handshake_timeout,
							  std::chrono::milliseconds tls_read_timeout, std::chrono::milliseconds tls_write_timeout,
							  bool server_verification_flag);

			OpenSSLConnection(util::String endpoint, uint16_t endpoint_port, util::String root_ca_location,
							  std::chrono::milliseconds tls_handshake_timeout,
							  std::chrono::milliseconds tls_read_timeout, std::chrono::milliseconds tls_write_timeout,
							  bool server_verification_flag);

			ResponseCode Initialize();

			/**
			 * @brief Check if TLS layer is still connected
			 *
			 * Called to check if the TLS layer is still connected or not.
			 *
			 * @return bool - indicating status of network TLS layer connection
			 */
			bool IsConnected();

			/**
			 * @brief Check if Network Physical layer is still connected
			 *
			 * Called to check if the Network Physical layer is still connected or not.
			 *
			 * @return bool - indicating status of network physical layer connection
			 */
			bool IsPhysicalLayerConnected();

			virtual ~OpenSSLConnection();
		};
	}
}
