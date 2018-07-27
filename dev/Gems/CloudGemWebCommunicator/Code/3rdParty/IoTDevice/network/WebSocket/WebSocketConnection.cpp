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
 * @file WebSocketConnection.cpp
 * @brief
 *
 */

#include "StdAfx.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32")
#endif
#include <iostream>
#include <thread>
#include <iterator>
#include <ctime>
#include <random>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <cstring>

#include <aws/core/utils/StringUtils.h>

#include "WebSocketConnection.hpp"
#include "util/logging/LogMacros.hpp"

#define AWS_IOT_DATA_SERVICE_NAME "iotdata"
#define SLASH_URLENCODE "%2F"
#define AWS_HMAC_SHA256 "AWS4-HMAC-SHA256"
#define AWS4_REQUEST "aws4_request"
#define X_AMZ_SIGNED_HEADERS "X-Amz-SignedHeaders"
#define X_AMZ_ALGORITHM "X-Amz-Algorithm"
#define X_AMZ_CREDENTIAL "X-Amz-Credential"
#define X_AMZ_SIGNATURE "X-Amz-Signature"
#define X_AMZ_DATE "X-Amz-Date"
#define X_AMZ_EXPIRES "X-Amz-Expires"
#define X_AMZ_SECURITY_TOKEN "X-Amz-Security-Token"
#define SIGNING_KEY "AWS4"
#define LONG_DATE_FORMAT_STR "%Y%m%dT%H%M%SZ"
#define SIMPLE_DATE_FORMAT_STR "%Y%m%d"
#define METHOD "GET"
#define CANONICAL_URI "/mqtt"
#define EMPTY_BODY_SHA256 "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

#define MAX_LEN_FOR_UTCTIME 16
#define MAX_LEN_FOR_AWS_REGION 32
#define REGION_TOKEN_SEQUENCE 2
#define SIGNING_KEY_LEN 4
#define MAX_SIGNATURE_LEN EVP_MAX_MD_SIZE

#define CREDENTIAL_SCOPE_BUF_LEN 64
#define CREDENTIAL_SCOPE_URL_ENCODE_BUF_LEN 64
#define CANONICAL_QUERY_BUF_LEN 512
#define CANONICAL_HEADER_BUF_LEN 64
#define CANONICAL_REQUEST_BUF_LEN 512
#define STRING_TO_SIGN_BUF_LEN 512

#define WEBSOCKET_PROTOCOL_ENDPOINT_PREFIX "wss://"
#define WEBSOCKET_WRAPPER_LOG_TAG "[WebSocket Wrapper]"
#define MAX_WSS_SIGNED_ENDPOINT_LEN 512

#define HTTP_1_1 "HTTP/1.1"
#define UPGRADE "Upgrade"
#define WEBSOCKET "WebSocket"
#define SEC_WEBSOCKET_VERSION_13 "13"
#define MQTT_PROTOCOL "mqttv3.1.1"
#define WSSGUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WSS_SUCCESS_HANDSHAKE_RESP_HEADER "sec-websocket-accept"

#define TO_HASH_BUF_LEN 64
#define WSS_CLIENT_KEY_MAX_LEN 64
#define RANDOM_BYTES_LEN 16
#define SERVER_WSS_ACCEPT_KEY_LEN 28
#define SERVER_WSS_RESP_LEN 182

#pragma warning(disable : 4996)

namespace awsiotsdk {
	namespace network {
		std::mutex WebSocketConnection::time_ops_lock_;

		WebSocketConnection::WebSocketConnection(util::String endpoint, uint16_t endpoint_port,
												 util::String root_ca_location, util::String aws_region,
												 util::String aws_access_key_id, util::String aws_secret_access_key,
												 util::String aws_session_token,
												 std::chrono::milliseconds tls_handshake_timeout,
												 std::chrono::milliseconds tls_read_timeout,
												 std::chrono::milliseconds tls_write_timeout,
												 bool server_verification_flag)
			:openssl_connection_(endpoint, endpoint_port, root_ca_location, tls_handshake_timeout, tls_read_timeout,
								 tls_write_timeout, server_verification_flag) {
			endpoint_ = endpoint;
			endpoint_port_ = endpoint_port;
			root_ca_location_ = root_ca_location;
			aws_access_key_id_ = aws_access_key_id;
			aws_secret_access_key_ = aws_secret_access_key;
			aws_session_token_ = aws_session_token;
			aws_region_ = aws_region;

			is_connected_ = false;
			curr_read_buf_size_ = 0;

			p_wslay_frame_Callbacks_ = new wslay_frame_callbacks();
			p_wslay_frame_Callbacks_->send_callback = std::bind(&WebSocketConnection::WssFrameSendCallback, this,
																std::placeholders::_1, std::placeholders::_2,
																std::placeholders::_3, std::placeholders::_4);
			p_wslay_frame_Callbacks_->recv_callback = std::bind(&WebSocketConnection::WssFrameRecvCallback, this,
																std::placeholders::_1, std::placeholders::_2,
																std::placeholders::_3, std::placeholders::_4);
			p_wslay_frame_Callbacks_->genmask_callback = std::bind(&WebSocketConnection::WssFrameGenMaskCallback, this,
																   std::placeholders::_1, std::placeholders::_2,
																   std::placeholders::_3);

			wss_frame_read_ = std::unique_ptr<wslay_frame_iocb>(new wslay_frame_iocb());
			wss_frame_write_ = std::unique_ptr<wslay_frame_iocb>(new wslay_frame_iocb());
		}

		ResponseCode WebSocketConnection::ConnectInternal() {
			// Init Tls
			ResponseCode rc = openssl_connection_.Initialize();
			if(ResponseCode::SUCCESS != rc) {
				AWS_LOG_ERROR(WEBSOCKET_WRAPPER_LOG_TAG, "SSL Initialize failed");
				return rc;
			}

			// Connect Tls
			rc = openssl_connection_.Connect();
			if(ResponseCode::SUCCESS != rc) {
				AWS_LOG_ERROR(WEBSOCKET_WRAPPER_LOG_TAG, "SSL Connect failed");
				return rc;
			}

			// WebSocket Http Handshake
			rc = WssHandshake();
			if(ResponseCode::SUCCESS != rc) {
				AWS_LOG_ERROR(WEBSOCKET_WRAPPER_LOG_TAG, "WebSocket Handshake failed");
				return rc;
			}

			// Set up wslay frame context
			if(wslay_frame_context_init(&p_wslay_frame_Context_, p_wslay_frame_Callbacks_, nullptr) < 0) {
				return ResponseCode::WEBSOCKET_WSLAY_CONTEXT_INIT_ERROR;
			}

			is_connected_ = true;
			read_buf_.clear();

			return rc;
		}

		ResponseCode WebSocketConnection::ReadInternal(util::Vector<unsigned char> &buf, size_t buf_read_offset,
													   size_t size_bytes_to_read, size_t &size_read_bytes_out) {
			ResponseCode ret_code = ResponseCode::SUCCESS;
			bool continue_polling = true;

			do {
				// See if we already have enough bytes for this read request
				if (curr_read_buf_size_ >= size_bytes_to_read) {
					auto in_buf_itr = std::next(buf.begin(), buf_read_offset);
					if (in_buf_itr != buf.end()) {
						buf.erase(in_buf_itr, buf.end());
					}
					// Yes we have. Retrieve from the buffer and update the buffer status
					std::vector<unsigned char>::iterator itr = std::next(read_buf_.begin(), size_bytes_to_read);
					std::move(read_buf_.begin(), itr, std::back_inserter(buf));
					read_buf_.erase(read_buf_.begin(), itr);

					// Update buffer status
					curr_read_buf_size_ -= size_bytes_to_read;
					size_read_bytes_out = size_bytes_to_read;
					continue_polling = false;
				} else {
					// Unfortunately we don't. Retrieve a new wss frame from network
					wslay_frame_iocb *new_ws_frame = static_cast<wslay_frame_iocb *> (wss_frame_read_.get());
					ssize_t ws_read_res = wslay_frame_recv(p_wslay_frame_Context_, new_ws_frame);
					if (ws_read_res < 0) {
						ClearBuffer(); // Force a new ws frame
						//is_connected_ = false;
						ret_code = ResponseCode::WEBSOCKET_FRAME_RECEIVE_ERROR;
						continue_polling = false;
					} else if (ViolateServerToClientWsProtocol(new_ws_frame)) {
						ClearBuffer();
						//is_connected_ = false;
						ret_code = ResponseCode::WEBSOCKET_PROTOCOL_VIOLATION;
						continue_polling = false;
					} else if (WSLAY_CONNECTION_CLOSE == new_ws_frame->opcode) {
						ClearBuffer();
						//is_connected_ = false;
						ret_code = ResponseCode::WEBSOCKET_MAX_LIFETIME_REACHED;
						continue_polling = false;
					} else if (WSLAY_PING == new_ws_frame->opcode) {
						SendPongFromClient();
					} else if (WSLAY_PONG == new_ws_frame->opcode) {
						// Ignore this PONG and receive the next ws frame
					} else {
						AppendBytesToBuffer((char *) (new_ws_frame->data), new_ws_frame->data_length);
					}
				}
			} while (continue_polling);

			return ret_code;
		}

		size_t WebSocketConnection::AppendBytesToBuffer(const char *dest_buf, size_t num_bytes_to_append) {
			size_t itr;
			for (itr = 0; itr < num_bytes_to_append; itr++) {
				read_buf_.push_back((unsigned char) dest_buf[itr]);
			}
			curr_read_buf_size_ += num_bytes_to_append;
			return num_bytes_to_append;
		}

		void WebSocketConnection::ClearBuffer() {
			read_buf_.clear();
			curr_read_buf_size_ = 0;
		}

		bool WebSocketConnection::ViolateServerToClientWsProtocol(wslay_frame_iocb *new_ws_frame) {
			return new_ws_frame->rsv != 0 || new_ws_frame->mask != 0;
		}

		void WebSocketConnection::SendPongFromClient() {
			std::unique_ptr<wslay_frame_iocb> pong_ws_frame_uptr = std::unique_ptr<wslay_frame_iocb>();
			wslay_frame_iocb *pong_ws_frame = pong_ws_frame_uptr.get();
			EncodeWsFrameAsFinNoRsvNoExt(pong_ws_frame, WSLAY_PONG, 1, (unsigned char *)(""), 0);
			wslay_frame_send(p_wslay_frame_Context_, pong_ws_frame);
		}

		ResponseCode WebSocketConnection::WriteInternal(const util::String &buf, size_t &size_written_bytes_out) {
			ResponseCode ret_code = ResponseCode::SUCCESS;
			size_t size_bytes_to_write = buf.length();
			// Per-message deflate requires a complete Mqtt packet being packed into ONE ws frame
			wslay_frame_iocb *new_ws_frame = static_cast<wslay_frame_iocb *>(wss_frame_write_.get());
			const unsigned char *buf_temp = (const unsigned char *)(buf.c_str());
			EncodeWsFrameAsFinNoRsvNoExt(new_ws_frame, WSLAY_BINARY_FRAME, 1, buf_temp, size_bytes_to_write);

			// Send out this ws frame
			ssize_t data_len_sent = wslay_frame_send(p_wslay_frame_Context_, new_ws_frame);

			if((size_t) data_len_sent < size_bytes_to_write) {
				ret_code = ResponseCode::WEBSOCKET_FRAME_TRANSMIT_ERROR;
			} else {
				size_written_bytes_out = size_bytes_to_write;
			}

			return ret_code;
		}

		void WebSocketConnection::EncodeWsFrameAsFinNoRsvNoExt(wslay_frame_iocb *new_ws_frame, uint8_t op_code,
															   uint8_t mask, const unsigned char *data,
															   size_t data_len) {
			new_ws_frame->opcode = op_code;
			new_ws_frame->data_length = data_len;
			new_ws_frame->data = data;
			new_ws_frame->fin = 1;
			new_ws_frame->mask = mask;
			new_ws_frame->payload_length = data_len;
			new_ws_frame->rsv = 0;
		}

        ResponseCode WebSocketConnection::DisconnectInternal() {
            is_connected_ = false;
            ClearBuffer();
            ResponseCode rc = openssl_connection_.Disconnect();
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(WEBSOCKET_WRAPPER_LOG_TAG, "SSL Disconnect failed");
                return rc;
            }

			return rc;
		}

		bool WebSocketConnection::IsConnected() {
			return is_connected_;
		}

		bool WebSocketConnection::IsPhysicalLayerConnected() {
			return openssl_connection_.IsPhysicalLayerConnected();
		}

		WebSocketConnection::~WebSocketConnection() {
			delete p_wslay_frame_Callbacks_;
			wslay_frame_context_free(p_wslay_frame_Context_);
		}

		void WebSocketConnection::InitializeCredentialScope(const char *date_stamp, size_t date_stamp_len,
															util::String &credential_scope,
															util::String &credential_scope_url_encode) const {
			// Create credential scope
			credential_scope.reserve(CREDENTIAL_SCOPE_BUF_LEN);
			credential_scope.append(date_stamp, date_stamp_len);
			credential_scope.append("/");
			credential_scope.append(aws_region_);
			credential_scope.append("/");
			credential_scope.append(AWS_IOT_DATA_SERVICE_NAME);
			credential_scope.append("/");
			credential_scope.append(AWS4_REQUEST);

			AWS_LOG_DEBUG(WEBSOCKET_WRAPPER_LOG_TAG, "Credential Scope: %s", credential_scope.c_str());

			// Create credential scope url encoded
			credential_scope_url_encode.reserve(CREDENTIAL_SCOPE_URL_ENCODE_BUF_LEN);
			credential_scope_url_encode.append(date_stamp);
			credential_scope_url_encode.append(SLASH_URLENCODE);
			credential_scope_url_encode.append(aws_region_);
			credential_scope_url_encode.append(SLASH_URLENCODE);
			credential_scope_url_encode.append(AWS_IOT_DATA_SERVICE_NAME);
			credential_scope_url_encode.append(SLASH_URLENCODE);
			credential_scope_url_encode.append(AWS4_REQUEST);
		}

		void WebSocketConnection::InitializeSigningKey(const char *date_stamp, size_t date_stamp_len,
													   util::Vector<unsigned char> &sig_key,
													   unsigned int &sig_key_len) const {
			sig_key_len = 0;
			util::String initial_secret;
			initial_secret.reserve(aws_secret_access_key_.length() + SIGNING_KEY_LEN);
			initial_secret.append(SIGNING_KEY, SIGNING_KEY_LEN);
			initial_secret.append(aws_secret_access_key_);

			// Get signature key
			util::Vector<unsigned char> signing_date;
			signing_date.resize(MAX_SIGNATURE_LEN + 1);
			util::Vector<unsigned char> signing_region;
			signing_region.resize(MAX_SIGNATURE_LEN + 1);
			util::Vector<unsigned char> signing_service;
			signing_service.resize(MAX_SIGNATURE_LEN + 1);
			sig_key.resize(MAX_SIGNATURE_LEN + 1);
			unsigned int signing_date_len = 0;
			unsigned int signing_region_len = 0;
			unsigned int signing_service_len = 0;
			HMAC(EVP_sha256(), (const void *) initial_secret.c_str(), (int) initial_secret.length(),
				 (const unsigned char *) date_stamp, date_stamp_len,
				 &signing_date[0], &signing_date_len);

			HMAC(EVP_sha256(), (const void *) &signing_date[0], (int) signing_date_len,
				 (const unsigned char *) aws_region_.c_str(), aws_region_.length(),
				 &signing_region[0], &signing_region_len);

			util::String iot_data_service_name(AWS_IOT_DATA_SERVICE_NAME);
			HMAC(EVP_sha256(), (const void *) &signing_region[0], (int) signing_region_len,
				 (const unsigned char *) iot_data_service_name.c_str(), iot_data_service_name.length(),
				 &signing_service[0], &signing_service_len);

			util::String aws4_request(AWS4_REQUEST);
			HMAC(EVP_sha256(), (const void *) &signing_service[0], (int) signing_service_len,
				 (const unsigned char *) aws4_request.c_str(), aws4_request.length(),
				 &sig_key[0], &sig_key_len);
		}

		void WebSocketConnection::InitializeSignedString(const char *amz_date, const char *date_stamp,
														 size_t date_stamp_len, size_t amz_date_len,
														 const util::String &credential_scope,
														 const util::String &canonical_request,
														 util::Vector<unsigned char> &signed_str,
														 unsigned int &signed_string_len) const {
			// -> Get hash value for canonical request
			util::Vector<unsigned char> hashed_canonical_request;
			hashed_canonical_request.resize(SHA256_DIGEST_LENGTH);
			SHA256((const unsigned char *) canonical_request.c_str(), canonical_request.length(),
				   &hashed_canonical_request[0]);

			// -> Get string to sign
			util::String string_to_sign;
			string_to_sign.reserve(STRING_TO_SIGN_BUF_LEN);

			string_to_sign.append(AWS_HMAC_SHA256);
			string_to_sign.append("\n");
			string_to_sign.append(amz_date, amz_date_len);
			string_to_sign.append("\n");
			string_to_sign.append(credential_scope);
			string_to_sign.append("\n");

			// -> Convert hash value to hex string
			char temp[3];
			for (unsigned char c : hashed_canonical_request) {
				// LY 2013 fix
				azsnprintf(temp, 3, "%02x", c);
				string_to_sign.append(temp, 2);
			}

			AWS_LOG_DEBUG(WEBSOCKET_WRAPPER_LOG_TAG, "StringToSign: %s", string_to_sign.c_str());

			util::Vector<unsigned char> sig_key;
			unsigned int sig_key_len;
			InitializeSigningKey(date_stamp, date_stamp_len, sig_key, sig_key_len);

			// Sign the string
			HMAC(EVP_sha256(), (const void *) &sig_key[0], (int) sig_key_len,
				 (const unsigned char *) string_to_sign.c_str(), string_to_sign.length(),
				 &signed_str[0], &signed_string_len);
		}

		ResponseCode WebSocketConnection::InitializeCanonicalQueryString(util::String &canonical_query_string) const {
			char amz_date[MAX_LEN_FOR_UTCTIME + 1];
			char date_stamp[MAX_LEN_FOR_UTCTIME + 1];
			size_t date_stamp_len;
			size_t amz_date_len;

			{
				// C Style time functions are NOT thread safe, need locking
				std::lock_guard<std::mutex> time_ops_guard(time_ops_lock_);

				// Get current system time
				auto now = std::chrono::system_clock::now();
				std::time_t current_time = std::chrono::system_clock::to_time_t(now);

				// Get time, not required to free this
				struct tm *curr_utc_time = gmtime(&current_time);
				if(nullptr == curr_utc_time) {
					AWS_LOG_ERROR(WEBSOCKET_WRAPPER_LOG_TAG, "Failed to get UTC Date!!");
					return ResponseCode::WEBSOCKET_GET_UTC_TIME_FAILED;
				}
				amz_date_len = std::strftime(amz_date, MAX_LEN_FOR_UTCTIME + 1, LONG_DATE_FORMAT_STR, curr_utc_time);
				date_stamp_len =
					std::strftime(date_stamp, MAX_LEN_FOR_UTCTIME + 1, SIMPLE_DATE_FORMAT_STR, curr_utc_time);
				if(0 == amz_date_len || 0 == date_stamp_len) {
					AWS_LOG_ERROR(WEBSOCKET_WRAPPER_LOG_TAG, "Failed to convert UTC date to required format!!");
					return ResponseCode::WEBSOCKET_GET_UTC_TIME_FAILED;
				}
				AWS_LOG_DEBUG(WEBSOCKET_WRAPPER_LOG_TAG, "AmzDate: %s  DateStamp: %s", amz_date, date_stamp);
			}

			AWS_LOG_DEBUG(WEBSOCKET_WRAPPER_LOG_TAG, "Region: %s", aws_region_.c_str());
			util::String credential_scope;
			util::String credential_scope_url_encode;
			InitializeCredentialScope(date_stamp, date_stamp_len, credential_scope, credential_scope_url_encode);

			AWS_LOG_DEBUG(WEBSOCKET_WRAPPER_LOG_TAG, "Credential Scope Url Encoded: %s",
						  credential_scope_url_encode.c_str());

			// X-Amz-Algorithm
			canonical_query_string.append(X_AMZ_ALGORITHM);
			canonical_query_string.append("=");
			canonical_query_string.append(AWS_HMAC_SHA256);

			// X-Amz-Credential
			canonical_query_string.append("&");
			canonical_query_string.append(X_AMZ_CREDENTIAL);
			canonical_query_string.append("=");
			canonical_query_string.append(aws_access_key_id_);
			canonical_query_string.append(SLASH_URLENCODE);
			canonical_query_string.append(credential_scope_url_encode);

			// -> X-Amz-Date
			canonical_query_string.append("&");
			canonical_query_string.append(X_AMZ_DATE);
			canonical_query_string.append("=");
			canonical_query_string.append(amz_date, amz_date_len);

			// -> X-Amz-Expires
			canonical_query_string.append("&");
			canonical_query_string.append(X_AMZ_EXPIRES);
			canonical_query_string.append("=86400");

			// -> X-Amz-SignedHeaders
			canonical_query_string.append("&");
			canonical_query_string.append(X_AMZ_SIGNED_HEADERS);
			canonical_query_string.append("=host");
			AWS_LOG_DEBUG(WEBSOCKET_WRAPPER_LOG_TAG, "CanonicalQuery: %s", canonical_query_string.c_str());

			// Create canonical header string
			util::String canonical_headers;
			canonical_headers.reserve(CANONICAL_HEADER_BUF_LEN);
			canonical_headers.append("host:");
			canonical_headers.append(endpoint_);
			AWS_LOG_DEBUG(WEBSOCKET_WRAPPER_LOG_TAG, "CanonicalHeaders: %s", canonical_headers.c_str());

			// Create canonical request
			util::String canonical_request;
			canonical_request.reserve(CANONICAL_REQUEST_BUF_LEN);
			canonical_request.append(METHOD);
			canonical_request.append("\n");
			canonical_request.append(CANONICAL_URI);
			canonical_request.append("\n");
			canonical_request.append(canonical_query_string);
			canonical_request.append("\n");
			canonical_request.append(canonical_headers);
			canonical_request.append("\n\nhost\n");
			canonical_request.append(EMPTY_BODY_SHA256);
			AWS_LOG_DEBUG(WEBSOCKET_WRAPPER_LOG_TAG, "CanonicalRequest: %s", canonical_request.c_str());

			// Create string to sign
			util::Vector<unsigned char> signed_str;
			signed_str.resize(MAX_SIGNATURE_LEN);
			unsigned int signed_string_len = 0;
			InitializeSignedString(amz_date, date_stamp, date_stamp_len, amz_date_len, credential_scope,
								   canonical_request, signed_str, signed_string_len);

			// Complete canonical query string
			canonical_query_string.append("&");
			canonical_query_string.append(X_AMZ_SIGNATURE);
			canonical_query_string.append("=");

			size_t itr;
			unsigned char c;
			char temp[3];
			for(itr = 0; itr < signed_string_len; itr++) {
				c = signed_str[itr];
				// LY 2013 fix
				azsnprintf(temp, 3, "%02x", c);
				canonical_query_string.append(temp, 2);
			}

			// -> Check session token
			if(0 < aws_session_token_.length()) {
				canonical_query_string.append("&");
				canonical_query_string.append(X_AMZ_SECURITY_TOKEN);
				canonical_query_string.append("=");
				canonical_query_string.append(Aws::Utils::StringUtils::URLEncode(aws_session_token_.c_str()).c_str());
			}
			AWS_LOG_DEBUG(WEBSOCKET_WRAPPER_LOG_TAG, "CompletedCanonicalQuery: %s", canonical_query_string.c_str());
			return ResponseCode::SUCCESS;
		}

        int WebSocketConnection::WssFrameSendCallback(const uint8_t* data, size_t len, int flags, void* user_data) {
			util::String out_data((char *)data, len);
			ResponseCode rc = WriteToNetworkBuffer(out_data);
			if(ResponseCode::SUCCESS != rc) {
				AWS_LOG_ERROR(WEBSOCKET_WRAPPER_LOG_TAG, "SSL Write failed with error code : %d", static_cast<int>(rc));
				return -1;
			}
			return (int)len;
		}

        int WebSocketConnection::WssFrameRecvCallback(uint8_t* buf, size_t bytes_to_read, int flags, void* user_data) {
			util::Vector <unsigned char> read_buf;
			ResponseCode rc = ReadFromNetworkBuffer(read_buf, bytes_to_read);

			if(ResponseCode::NETWORK_SSL_NOTHING_TO_READ == rc) {
				return 0;
			} else if(ResponseCode::SUCCESS != rc) {
				AWS_LOG_ERROR(WEBSOCKET_WRAPPER_LOG_TAG, "SSL Read failed with error code : %d", static_cast<int>(rc));
				return -1;
			}

			size_t itr = 0;
			for(unsigned char c : read_buf) {
				buf[itr] = (uint8_t)c;
				itr++;
			}

			return (int)bytes_to_read;
		}

		int WebSocketConnection::WssFrameGenMaskCallback(uint8_t* buf, size_t len, void* user_data) {
			return GetRandomBytesOfLength(buf, len);
		}

		ResponseCode WebSocketConnection::WssHandshake() {
			// Assuming:
			// 1. Ssl socket is ready to do read/write.

			// Create canonical query string
			util::String canonical_query_string;
			canonical_query_string.reserve(CANONICAL_QUERY_BUF_LEN);
			ResponseCode rc = InitializeCanonicalQueryString(canonical_query_string);
			if(ResponseCode::SUCCESS != rc) {
				return rc;
			}

			// Create Wss handshake Http request
			// -> Generate Wss client key
			char client_key_buf[WSS_CLIENT_KEY_MAX_LEN + 1];
			size_t client_key_len = 0;
			rc = GenerateClientKey(client_key_buf, &client_key_len);
			if(ResponseCode::SUCCESS != rc) {
				return rc;
			}
            

            // -> Assemble Wss Http request
            util::OStringStream stringStream;
            stringStream << "GET /mqtt?" << canonical_query_string << " " << HTTP_1_1 << "\r\n"
                         << "Host: " << endpoint_ << "\r\n"
                         << "Connection: " << UPGRADE << "\r\n"
                         << "Upgrade: " << WEBSOCKET << "\r\n"
                         << "Sec-WebSocket-Version: " << SEC_WEBSOCKET_VERSION_13 << "\r\n"
                         << "sec-websocket-key: " << client_key_buf << "\r\n"
                         << "Sec-WebSocket-Protocol: " << MQTT_PROTOCOL << "\r\n\r\n";
            util::String request_string = stringStream.str();

            

            // Send out request
            rc = WriteToNetworkBuffer(request_string);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(WEBSOCKET_WRAPPER_LOG_TAG,
                              "SSL Write failed, %d",
                              rc);
                return rc;
            }

            // Retrieve response
            util::Vector<unsigned char> r_buf;
            r_buf.resize(SERVER_WSS_RESP_LEN);
            rc = ReadFromNetworkBuffer(r_buf, SERVER_WSS_RESP_LEN);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(WEBSOCKET_WRAPPER_LOG_TAG,
                              "SSL Read failed, %d",
                             rc);
                return rc;
            }

            // Verify and return handshake result
            return VerifyHandshakeResponse(r_buf, SERVER_WSS_RESP_LEN, client_key_buf);
        }

		ResponseCode WebSocketConnection::GenerateClientKey(char* res_buf, size_t* res_len) {
			unsigned char random_bytes_16[RANDOM_BYTES_LEN];
			int isRandomGot = GetRandomBytesOfLength(random_bytes_16, RANDOM_BYTES_LEN);
			if(isRandomGot < 0) {
				return ResponseCode::WEBSOCKET_GEN_CLIENT_KEY_ERROR;
			}
			Base64Encode(res_buf, res_len, random_bytes_16, RANDOM_BYTES_LEN);

			return ResponseCode::SUCCESS;
		}

        ResponseCode WebSocketConnection::VerifyHandshakeResponse(const util::Vector<unsigned char> &resp,
                                                                  size_t resp_len,
                                                                  const char *client_key) {
            // Verify accept
            util::String resp_buf_str((char *) &resp[0], resp_len);
            size_t success_header_start_index = resp_buf_str.find(WSS_SUCCESS_HANDSHAKE_RESP_HEADER);
            if (util::String::npos == success_header_start_index) {
                return ResponseCode::WEBSOCKET_HANDSHAKE_VERIFY_ERROR;
            }

			// Verify accept key
			util::String server_accept_key
				= resp_buf_str.substr(success_header_start_index + strlen(WSS_SUCCESS_HANDSHAKE_RESP_HEADER) + 2,
									  SERVER_WSS_ACCEPT_KEY_LEN);
			if(0 != VerifyWssAcceptKey(server_accept_key.c_str(), client_key)) {
				return ResponseCode::WEBSOCKET_HANDSHAKE_VERIFY_ERROR;
			}

			return ResponseCode::SUCCESS;
		}

		int WebSocketConnection::VerifyWssAcceptKey(const char* accept_key, const char* client_key) {
			char client_gen_accept_key_buf[WSS_CLIENT_KEY_MAX_LEN + 1];
			char to_hash_buf[TO_HASH_BUF_LEN];
			// LY 2013 Fix
			azsnprintf(to_hash_buf, TO_HASH_BUF_LEN, "%s%s", client_key, WSSGUID);

			size_t client_gen_accept_key_len = 0;
			unsigned char sha1_res_buf[SHA_DIGEST_LENGTH];
			SHA1((unsigned char*)to_hash_buf, strnlen(to_hash_buf, TO_HASH_BUF_LEN), sha1_res_buf);
			Base64Encode(client_gen_accept_key_buf, &client_gen_accept_key_len, sha1_res_buf, SHA_DIGEST_LENGTH);

			return strncmp(accept_key, client_gen_accept_key_buf, client_gen_accept_key_len);
		}

		void WebSocketConnection::Base64Encode(char* res_buf, size_t* res_len, const unsigned char* buf_in, size_t buf_in_data_len) {
			BIO* mem_buf, *b64_func;
			BUF_MEM* mem_struct;

			b64_func = BIO_new(BIO_f_base64());
			mem_buf = BIO_new(BIO_s_mem());
			mem_buf = BIO_push(b64_func, mem_buf);

			BIO_set_flags(mem_buf, BIO_FLAGS_BASE64_NO_NL);
			int rc = BIO_set_close(mem_buf, BIO_CLOSE);
			IOT_UNUSED(rc);
			BIO_write(mem_buf, buf_in, (int)buf_in_data_len);
			rc = BIO_flush(mem_buf);
			IOT_UNUSED(rc);

			BIO_get_mem_ptr(mem_buf, &mem_struct);
			memcpy(res_buf, mem_struct->data, mem_struct->length);
			*res_len = mem_struct->length;
			res_buf[*res_len] = '\0';

			BIO_free_all(mem_buf);
		}

		ResponseCode WebSocketConnection::ReadFromNetworkBuffer(util::Vector<unsigned char> &read_buf, size_t bytes_to_read) {
			size_t total_read_bytes = 0;

			read_buf.resize(bytes_to_read);
			// TODO: Add support for partial reads
			ResponseCode rc = openssl_connection_.Read(read_buf, 0, bytes_to_read, total_read_bytes);

			if(ResponseCode::SUCCESS == rc && total_read_bytes != bytes_to_read) {
				rc = ResponseCode::NETWORK_SSL_READ_ERROR;
			}

			return rc;
		}

		ResponseCode WebSocketConnection::WriteToNetworkBuffer(const util::String &write_buf) {
			if(0 == write_buf.length()) {
				return ResponseCode::NETWORK_NOTHING_TO_WRITE_ERROR;
			}

			size_t total_written_bytes = 0;
			size_t bytes_to_write = write_buf.length();
			// TODO: Add support for partial writes
			ResponseCode rc = openssl_connection_.Write(write_buf, total_written_bytes);

			if(ResponseCode::SUCCESS == rc && total_written_bytes != bytes_to_write) {
				rc = ResponseCode::NETWORK_SSL_WRITE_ERROR;
			}

			return rc;
		}

		int WebSocketConnection::GetRandomBytesOfLength(unsigned char* res_buf, size_t len) {
			std::random_device rd;
			size_t itr;
			for(itr = 0; itr < len; itr++) {
				res_buf[itr] = (unsigned char) (rd() % (1<<8)); // Random byte
			}

			return 0;
		}
	}
}