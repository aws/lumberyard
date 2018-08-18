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
 * @file WebSocketConnection.hpp
 * @brief
 *
 */

#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <network/OpenSSL/OpenSSLConnection.hpp>

#include "wslay/wslay.hpp"
#include "NetworkConnection.hpp"
#include "ResponseCode.hpp"

namespace awsiotsdk {
	namespace network {
		/**
		 * @brief WebSocket for MQTT Wrapper Class
		 *
		 * Defines a wrapper for WebSocket for MQTT libraries on Linux
		 */
		class WebSocketConnection : public NetworkConnection {
		protected:
			util::String root_ca_location_;                      ///< Pointer to string containing the filename (including path) of the root CA file.
			util::String aws_access_key_id_;                     ///< Pointer to string containing the AWS Access Key Id.
			util::String aws_secret_access_key_;                 ///< Pointer to sstring containing the AWS Secret Access Key.
			util::String aws_session_token_;                     ///< Pointer to string containing the AWS Session Token.
			util::String aws_region_;                                ///< Region for this connection
			util::String endpoint_;                                ///< Endpoint for this connection
			uint16_t endpoint_port_;                            ///< Endpoint port

			OpenSSLConnection openssl_connection_;
			bool is_connected_;                                    ///< Boolean indicating connection status

			wslay_frame_context_ptr p_wslay_frame_Context_;     ///< WebSocket Context instance
			wslay_frame_callbacks *p_wslay_frame_Callbacks_;    ///< Websocket Callbacks

			// Memory alignment with Mqtt
			std::vector<unsigned char> read_buf_;               ///< WebSocket read frame decode buffer
			size_t curr_read_buf_size_;                         ///< WebSocket read frame decode buffer operation needle

			// Wss frame container
			std::unique_ptr<wslay_frame_iocb> wss_frame_read_;        ///< WebSocket frame struct for storing incoming frames
			std::unique_ptr<wslay_frame_iocb> wss_frame_write_;        ///< WebSocket frame struct for storing outgoing frames

			static std::mutex time_ops_lock_;
			/**
			 * @brief Append bytes to WebSocket decode buffer
			 *
			 * @param char pointer - pointer to buffer where bytes to be appended should be copied from
			 * @param size_t - number of bytes to append
			 * @return size_t - number of bytes appended
			 */
			size_t AppendBytesToBuffer(const char *dest_buf, size_t num_bytes_to_append);

			/**
			 * @brief Send a WebSocket PONG frame to the remote server
			 *
			 */
			void SendPongFromClient(void);

			/**
			 * @brief Encode a WebSocket frame as FIN frame with no RSV bits or EXT bits
			 *
			 * @param wslay frame pointer - pointer to wslay WebSocket frame struct where encoded bytes should be copied
			 * @param uint8_t - WebSocket op code
			 * @param uint8_t - WebSocket mask bit
			 * @param unsigned char pointer - pointer to buffer where bytes to be encoded should be retrieved from
			 * @param size_t - number of bytes to be encoded
			 */
			void EncodeWsFrameAsFinNoRsvNoExt(wslay_frame_iocb *new_ws_frame,
											  uint8_t op_code,
											  uint8_t mask,
											  const unsigned char *data,
											  size_t data_len);

			/**
			 * @brief Check if the received WebSocket frame from server has violated the protocol
			 *
			 * @param wslay frame pointer - pointer to wslay WebSocket frame struct
			 * @return bool - if this WebSocket frame has violated the protocol
			 */
			bool ViolateServerToClientWsProtocol(wslay_frame_iocb *new_ws_frame);

			/**
			 * @brief Clear the WebSocket decode buffer
			 *
			 */
			void ClearBuffer(void);

			/**
			 * @brief Create a WebSocket and negotiate the connection
			 *
			 * Creates an open WebSocket connection including TLS and WebSocket handshake.
			 *
			 * @return ResponseCode - successful connection or WebSocket error
			 */
			ResponseCode ConnectInternal();

			/**
			 * @brief Write bytes to the network WebSocket
			 *
			 * @param unsigned char pointer - buffer to write to WebSocket
			 * @param size_t - number of bytes to write
			 * @return size_t - number of bytes written or WebSocket error
			 * @return ResponseCode - successful write or WebSocket error code
			 */
			ResponseCode WriteInternal(const util::String &buf, size_t &size_written_bytes_out);

			/**
			 * @brief Read bytes from the network WebSocket
			 *
			 * @param unsigned char pointer - pointer to buffer where read bytes should be copied
			 * @param size_t - number of bytes to read
			 * @param size_t - pointer to store number of bytes read
			 * @return ResponseCode - successful read or WebSocket error code
			 */
			ResponseCode ReadInternal(util::Vector<unsigned char> &buf, size_t buf_read_offset,
									  size_t size_bytes_to_read, size_t &size_read_bytes_out);

			/**
			 * @brief Disconnect from network WebSocket
			 *
			 * @return ResponseCode - successful read or WebSocket error code
			 */
			ResponseCode DisconnectInternal();

			void InitializeCredentialScope(const char *date_stamp, size_t date_stamp_len,
										   util::String &credential_scope,
										   util::String &credential_scope_url_encode) const;
			void InitializeSigningKey(const char *date_stamp, size_t date_stamp_len,
									  util::Vector<unsigned char> &sig_key, unsigned int &sig_key_len) const;
			void InitializeSignedString(const char *amz_date, const char *date_stamp, size_t date_stamp_len,
										size_t amz_date_len, const util::String &credential_scope,
										const util::String &canonical_request, util::Vector<unsigned char> &signed_str,
										unsigned int &signed_string_len) const;

			ResponseCode InitializeCanonicalQueryString(util::String &canonical_query_string) const;

			int WssFrameSendCallback(const uint8_t* data, size_t len, int flags, void* user_data);

            int WssFrameRecvCallback(uint8_t* buf, size_t len, int flags, void* user_data);

			int WssFrameGenMaskCallback(uint8_t* buf, size_t len, void* user_data);

			ResponseCode WssHandshake();

			ResponseCode GenerateClientKey(char* res_buf, size_t* res_len);

			ResponseCode VerifyHandshakeResponse(const util::Vector<unsigned char> &resp, size_t resp_len, const char* client_key);

			int VerifyWssAcceptKey(const char* accept_key, const char* client_key);

			void Base64Encode(char* res_buf, size_t* res_len, const unsigned char* buf_in, size_t buf_in_data_len);

			ResponseCode ReadFromNetworkBuffer(util::Vector<unsigned char> &read_buf, size_t bytes_to_read);

			ResponseCode WriteToNetworkBuffer(const util::String &write_buf);

			int GetRandomBytesOfLength(unsigned char* res_buf, size_t len);
		public:
			/**
			 * @brief Constructor for the WebSocket for MQTT implementation
			 *
			 * Performs any initialization required by the WebSocket layer.
			 *
			 * @param util::String endpoint - The target endpoint to connect to
			 * @param uint16_t endpoint_port - The port on the target to connect to
			 * @param util::String root_ca_location - Path of the location of the Root CA
			 * @param util::String aws_access_key_id - AWS Access Key Id
			 * @param util::String aws_secret_access_key - AWS Secret Access Key
			 * @param util::String aws_session_token - AWS Session Token
			 * @param std::chrono::milliseconds tls_handshake_timeout - The value to use for timeout of handshake operation
			 * @param std::chrono::milliseconds tls_read_timeout - The value to use for timeout of read operation
			 * @param std::chrono::milliseconds tls_write_timeout - The value to use for timeout of write operation
			 * @param bool server_verification_flag - used to decide whether server verification is needed or not
			 *
			 */
			WebSocketConnection(util::String endpoint, uint16_t endpoint_port, util::String root_ca_location,
								util::String aws_region, util::String aws_access_key_id,
								util::String aws_secret_access_key, util::String aws_session_token,
								std::chrono::milliseconds tls_handshake_timeout,
								std::chrono::milliseconds tls_read_timeout, std::chrono::milliseconds tls_write_timeout,
								bool server_verification_flag);

			/**
			 * @brief Check if WebSocket layer is still connected
			 *
			 * Called to check if the WebSocket layer is still connected or not.
			 *
			 * @return bool - indicating status of network WebSocket layer connection
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

			virtual ~WebSocketConnection();
		};

	}
}
