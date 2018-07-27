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
 * @file MbedTLSConnection.hpp
 * @brief Defines a reference implementation for an MbedTLS library wrapper *
 */

#pragma once

#include "mbedtls/config.h"

#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"

#include "NetworkConnection.hpp"
#include "ResponseCode.hpp"

namespace awsiotsdk {
	namespace network {
		/**
		 * @brief MbedTLS Wrapper Class
		 *
		 * Defines a reference wrapper for MbedTLS libraries
		 */
		class MbedTLSConnection : public NetworkConnection {
		protected:
			util::String root_ca_location_;				///< Pointer to string containing the filename (including path) of the root CA file.
			util::String device_cert_location_;			///< Pointer to string containing the filename (including path) of the device certificate.
			util::String device_private_key_location_;	///< Pointer to string containing the filename (including path) of the device private key file.
			bool server_verification_flag_;				///< Boolean.  True = perform server certificate hostname validation.  False = skip validation \b NOT recommended.
			bool is_connected_;							///< Boolean indicating connection status
			std::chrono::milliseconds tls_handshake_timeout_;		///< Timeout for TLS handshake command
			std::chrono::milliseconds tls_read_timeout_;			///< Timeout for the TLS Read command
			std::chrono::milliseconds tls_write_timeout_;			///< Timeout for the TLS Write command

			// Endpoint information
			uint16_t endpoint_port_;					///< Endpoint port
			util::String endpoint_;						///< Endpoint for this connection

			mbedtls_entropy_context entropy_;
			mbedtls_ctr_drbg_context ctr_drbg_;
			mbedtls_ssl_context ssl_;
			mbedtls_ssl_config conf_;
			uint32_t flags_;
			mbedtls_x509_crt cacert_;
			mbedtls_x509_crt clicert_;
			mbedtls_pk_context pkey_;
			mbedtls_net_context server_fd_;

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
			static int VerifyCertificate(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags);

			/**
			 * @brief Constructor for the MbedTLS TLS implementation
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
			MbedTLSConnection(util::String endpoint, uint16_t endpoint_port, util::String root_ca_location,
							  util::String device_cert_location, util::String device_private_key_location,
							  std::chrono::milliseconds tls_handshake_timeout,
							  std::chrono::milliseconds tls_read_timeout, std::chrono::milliseconds tls_write_timeout,
							  bool server_verification_flag);

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

			virtual ~MbedTLSConnection();
		};
	}
}
