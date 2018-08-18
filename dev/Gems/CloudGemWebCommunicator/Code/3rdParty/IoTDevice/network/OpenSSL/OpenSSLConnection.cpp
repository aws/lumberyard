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
 * @brief
 *
 */
#include "StdAfx.h"

#include <iostream>
#include <util/memory/stl/Vector.hpp>

#include "OpenSSLConnection.hpp"
#include "util/logging/LogMacros.hpp"

#ifdef WIN32
#define MAX_PATH_LENGTH_ 260
#include <direct.h>
#define getcwd _getcwd // avoid MSFT "deprecation" warning
#else
#include <limits>
#define MAX_PATH_LENGTH_ PATH_MAX
#endif

#pragma warning(disable : 4996)

#define OPENSSL_WRAPPER_LOG_TAG "[OpenSSL Wrapper]"

namespace awsiotsdk {
	namespace network {
		OpenSSLConnection::OpenSSLConnection(util::String endpoint, uint16_t endpoint_port,
											 std::chrono::milliseconds tls_handshake_timeout,
											 std::chrono::milliseconds tls_read_timeout,
											 std::chrono::milliseconds tls_write_timeout,
											 bool server_verification_flag) {
			endpoint_ = endpoint;
			endpoint_port_ = endpoint_port;
			server_verification_flag_ = server_verification_flag;
			int timeout_ms = static_cast<int>(tls_handshake_timeout.count());
			tls_handshake_timeout_ = { timeout_ms / 1000, (timeout_ms % 1000) * 1000};
			timeout_ms = static_cast<int>(tls_read_timeout.count());
			tls_read_timeout_ = { timeout_ms / 1000, (timeout_ms % 1000) * 1000};
			timeout_ms = static_cast<int>(tls_write_timeout.count());
			tls_write_timeout_ = { timeout_ms / 1000, (timeout_ms % 1000) * 1000};

			is_connected_ = false;
		}

		OpenSSLConnection::OpenSSLConnection(util::String endpoint, uint16_t endpoint_port, util::String root_ca_location,
											 util::String device_cert_location, util::String device_private_key_location,
											 std::chrono::milliseconds tls_handshake_timeout,
											 std::chrono::milliseconds tls_read_timeout,
											 std::chrono::milliseconds tls_write_timeout,
											 bool server_verification_flag)
			:OpenSSLConnection(endpoint, endpoint_port, tls_handshake_timeout, tls_read_timeout, tls_write_timeout,
							   server_verification_flag) {
			root_ca_location_ = root_ca_location;
			device_cert_location_ = device_cert_location;
			device_private_key_location_ = device_private_key_location;
		}

		OpenSSLConnection::OpenSSLConnection(util::String endpoint, uint16_t endpoint_port, util::String root_ca_location,
											 std::chrono::milliseconds tls_handshake_timeout,
											 std::chrono::milliseconds tls_read_timeout,
											 std::chrono::milliseconds tls_write_timeout,
											 bool server_verification_flag)
			:OpenSSLConnection(endpoint, endpoint_port, tls_handshake_timeout, tls_read_timeout, tls_write_timeout,
							   server_verification_flag) {
			root_ca_location_ = root_ca_location;
			device_cert_location_.clear();
			device_private_key_location_.clear();
		}

		ResponseCode OpenSSLConnection::Initialize() {
#ifdef WIN32
			// TODO : Check if it is possible to replace this with std::call_once
			WSADATA wsa_data;
			int result;
			bool was_wsa_initialized = true;
			int s = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if(INVALID_SOCKET == s) {
				if(WSANOTINITIALISED == WSAGetLastError()) {
					was_wsa_initialized = false;
				}
			} else {
				closesocket(s);
			}

			if(!was_wsa_initialized) {
				// Initialize Winsock
				result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
				if(0 != result) {
					AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, "WSAStartup failed: %d", result);
					return ResponseCode::NETWORK_SSL_INIT_ERROR;
				}
			}
#endif

			const SSL_METHOD *method;

			OpenSSL_add_all_algorithms();
			ERR_load_BIO_strings();
			ERR_load_crypto_strings();
			SSL_load_error_strings();

			if(SSL_library_init() < 0) {
				return ResponseCode::NETWORK_SSL_INIT_ERROR;
			}

			method = TLSv1_2_method();

			if((p_ssl_context_ = SSL_CTX_new(method)) == NULL) {
				AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " SSL INIT Failed - Unable to create SSL Context");
				return ResponseCode::NETWORK_SSL_INIT_ERROR;
			}

			return ResponseCode::SUCCESS;
		}

		bool OpenSSLConnection::IsPhysicalLayerConnected() {
			// Use this to add implementation which can check for physical layer disconnect
			return true;
		}

		bool OpenSSLConnection::IsConnected() {
			return is_connected_;
		}

		ResponseCode OpenSSLConnection::ConnectTCPSocket() {
			const char *endpoint_char = endpoint_.c_str();
			if(nullptr == endpoint_char) {
				return ResponseCode::NETWORK_TCP_NO_ENDPOINT_SPECIFIED;
			}

			hostent *host = gethostbyname(endpoint_char);
			if(nullptr == host) {
				return ResponseCode::NETWORK_TCP_NO_ENDPOINT_SPECIFIED;
			}

			ResponseCode ret_val = ResponseCode::NETWORK_TCP_CONNECT_ERROR;
			sockaddr_in dest_addr;

			dest_addr.sin_family = AF_INET;
			dest_addr.sin_port = htons(endpoint_port_);
			dest_addr.sin_addr.s_addr = *(uint32_t *)(host->h_addr);
			memset(&(dest_addr.sin_zero), '\0', 8);

			int connect_status = connect(server_tcp_socket_fd_, (sockaddr *) &dest_addr, sizeof(sockaddr));
			if(-1 != connect_status) {
				ret_val = ResponseCode::SUCCESS;
			}

			return ret_val;
		}

		ResponseCode OpenSSLConnection::SetSocketToNonBlocking() {
			int status;
			ResponseCode ret_val = ResponseCode::SUCCESS;
#if defined(WIN32) || defined(WIN64)
			u_long flag = 1L;
			status = ioctlsocket(server_tcp_socket_fd_, FIONBIO, &flag);
			if (0 > status) {
				AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, "ioctlsocket - %s", strerror(errno));
				ret_val = ResponseCode::NETWORK_TCP_CONNECT_ERROR;
			}
#else
			int flags = fcntl(server_tcp_socket_fd_, F_GETFL, 0);
			// set underlying socket to non blocking
			if(0 > flags) {
				ret_val = ResponseCode::NETWORK_TCP_CONNECT_ERROR;
			}

			status = fcntl(server_tcp_socket_fd_, F_SETFL, flags | O_NONBLOCK);
			if(0 > status) {
				AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, "fcntl - %s", strerror(errno));
				ret_val = ResponseCode::NETWORK_TCP_CONNECT_ERROR;
			}
#endif

			return ret_val;
		}

		ResponseCode OpenSSLConnection::AttemptConnect() {
			ResponseCode ret_val = ResponseCode::FAILURE;
			int rc = 0;
			fd_set readFds;
			fd_set writeFds;
			struct timeval timeout = {tls_handshake_timeout_.tv_sec, tls_handshake_timeout_.tv_usec};
			int errorCode = 0;
			int select_retCode = 0;

			do {
				rc = SSL_connect(p_ssl_handle_);

				if(1 == rc) { //1 = SSL_CONNECTED, <= 0 is Error
					ret_val = ResponseCode::SUCCESS;
					break;
				}

				errorCode = SSL_get_error(p_ssl_handle_, rc);

				if(SSL_ERROR_WANT_READ == errorCode) {
					FD_ZERO(&readFds);
					FD_SET(server_tcp_socket_fd_, &readFds);
					select_retCode = select(server_tcp_socket_fd_ + 1, &readFds, NULL, NULL, &timeout);
					if(0 == select_retCode) { // 0 == SELECT_TIMEOUT
						AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " SSL Connect time out while waiting for read");
						ret_val = ResponseCode::NETWORK_SSL_CONNECT_TIMEOUT_ERROR;
					} else if(-1 == select_retCode) { // -1 == SELECT_ERROR
						AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " SSL Connect Select error for read %d", select_retCode);
						ret_val = ResponseCode::NETWORK_SSL_CONNECT_ERROR;
					}
				} else if(SSL_ERROR_WANT_WRITE == errorCode) {
					FD_ZERO(&writeFds);
					FD_SET(server_tcp_socket_fd_, &writeFds);
					select_retCode = select(server_tcp_socket_fd_ + 1, NULL, &writeFds, NULL, &timeout);
					if(0 == select_retCode) { // 0 == SELECT_TIMEOUT
						AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " SSL Connect time out while waiting for write");
						ret_val = ResponseCode::NETWORK_SSL_CONNECT_TIMEOUT_ERROR;
					} else if(-1 == select_retCode) { // -1 == SELECT_ERROR
						AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " SSL Connect Select error for write %d", select_retCode);
						ret_val = ResponseCode::NETWORK_SSL_CONNECT_ERROR;
					}
				} else {
					ret_val = ResponseCode::NETWORK_SSL_CONNECT_ERROR;
				}

			} while(ResponseCode::NETWORK_SSL_CONNECT_ERROR != ret_val &&
					ResponseCode::NETWORK_SSL_CONNECT_TIMEOUT_ERROR != ret_val);

			return ret_val;
		}

		ResponseCode OpenSSLConnection::ConnectInternal() {
			ResponseCode networkResponse = ResponseCode::SUCCESS;

			X509_VERIFY_PARAM *param = nullptr;

			server_tcp_socket_fd_ = (int)socket(AF_INET, SOCK_STREAM, 0);
			if(-1 == server_tcp_socket_fd_) {
				return ResponseCode::NETWORK_TCP_SETUP_ERROR;
			}

			AWS_LOG_DEBUG(OPENSSL_WRAPPER_LOG_TAG, "Root CA : %s", root_ca_location_.c_str());
			if(!SSL_CTX_load_verify_locations(p_ssl_context_, root_ca_location_.c_str(), NULL)) {
				AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " Root CA Loading error");
				return ResponseCode::NETWORK_SSL_ROOT_CRT_PARSE_ERROR;
			}

			if(0 < device_cert_location_.length() && 0 < device_private_key_location_.length()) {
				AWS_LOG_DEBUG(OPENSSL_WRAPPER_LOG_TAG, "Device crt : %s", device_cert_location_.c_str());
				if (!SSL_CTX_use_certificate_file(p_ssl_context_, device_cert_location_.c_str(), SSL_FILETYPE_PEM)) {
					AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " Device Certificate Loading error");
					return ResponseCode::NETWORK_SSL_DEVICE_CRT_PARSE_ERROR;
				}
				AWS_LOG_DEBUG(OPENSSL_WRAPPER_LOG_TAG, "Device privkey : %s", device_private_key_location_.c_str());
				if (1 != SSL_CTX_use_PrivateKey_file(p_ssl_context_,
													 device_private_key_location_.c_str(),
													 SSL_FILETYPE_PEM)) {
					AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " Device Private Key Loading error");
					return ResponseCode::NETWORK_SSL_KEY_PARSE_ERROR;
				}
			}

			p_ssl_handle_ = SSL_new(p_ssl_context_);

			// Requires OpenSSL v1.0.2 and above
			if(server_verification_flag_) {
				param = SSL_get0_param(p_ssl_handle_);
				// Enable automatic hostname checks
				X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
				X509_VERIFY_PARAM_set1_host(param, endpoint_.c_str(), 0);
			}

			// Configure a non-zero callback if desired
			SSL_set_verify(p_ssl_handle_, SSL_VERIFY_PEER, nullptr);

			networkResponse = ConnectTCPSocket();
			if(ResponseCode::SUCCESS != networkResponse) {
				AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, "TCP Connection error");
				return networkResponse;
			}

			SSL_set_fd(p_ssl_handle_, server_tcp_socket_fd_);

			networkResponse = SetSocketToNonBlocking();
			if(ResponseCode::SUCCESS != networkResponse) {
				AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " Unable to set the socket to Non-Blocking");
				return networkResponse;
			}

			networkResponse = AttemptConnect();
			if(X509_V_OK != SSL_get_verify_result(p_ssl_handle_)) {
				AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " Server Certificate Verification failed");
				networkResponse = ResponseCode::NETWORK_SSL_CONNECT_ERROR;
			} else {
				// ensure you have a valid certificate returned, otherwise no certificate exchange happened
				if(nullptr == SSL_get_peer_certificate(p_ssl_handle_)) {
					AWS_LOG_ERROR(OPENSSL_WRAPPER_LOG_TAG, " No certificate exchange happened");
					networkResponse = ResponseCode::NETWORK_SSL_CONNECT_ERROR;
				}
			}

			if(ResponseCode::SUCCESS == networkResponse) {
				is_connected_ = true;
			}

			return networkResponse;
		}

		ResponseCode OpenSSLConnection::WriteInternal(const util::String &buf, size_t &size_written_bytes_out) {
			int error_code = 0;
			int select_retCode = -1;
			int cur_written_length = 0;
			size_t total_written_length = 0;
			ResponseCode rc = ResponseCode::SUCCESS;
			fd_set write_fds;
			size_t bytes_to_write = buf.length();
			struct timeval timeout = {tls_write_timeout_.tv_sec, tls_write_timeout_.tv_usec};

			do {
				cur_written_length = SSL_write(p_ssl_handle_, buf.c_str(), bytes_to_write);
				error_code = SSL_get_error(p_ssl_handle_, cur_written_length);
				if(0 < cur_written_length) {
					total_written_length += (size_t) cur_written_length;
				} else if(SSL_ERROR_WANT_WRITE == error_code) {
					FD_ZERO(&write_fds);
					FD_SET(server_tcp_socket_fd_, &write_fds);
					select_retCode = select(server_tcp_socket_fd_ + 1, NULL, &write_fds, NULL, &timeout);
					if(0 == select_retCode) { //0 == SELECT_TIMEOUT
						rc = ResponseCode::NETWORK_SSL_WRITE_TIMEOUT_ERROR;
					} else if(-1 == select_retCode) { //-1 == SELECT_TIMEOUT
						rc = ResponseCode::NETWORK_SSL_WRITE_ERROR;
					}
				} else {
					rc = ResponseCode::NETWORK_SSL_WRITE_ERROR;
				}

			} while(is_connected_ && ResponseCode::NETWORK_SSL_WRITE_ERROR != rc &&
					ResponseCode::NETWORK_SSL_WRITE_TIMEOUT_ERROR != rc &&
					total_written_length < bytes_to_write);

			if(ResponseCode::SUCCESS == rc) {
				size_written_bytes_out = total_written_length;
			}

			return rc;
		}

		ResponseCode OpenSSLConnection::ReadInternal(util::Vector<unsigned char> &buf, size_t buf_read_offset,
													 size_t size_bytes_to_read, size_t &size_read_bytes_out) {
			int ssl_retcode;
			int select_retCode;
			size_t total_read_length = buf_read_offset;
			size_t remaining_bytes_to_read = size_bytes_to_read;
			int cur_read_len = 0;
			ResponseCode errorStatus = ResponseCode::SUCCESS;
			fd_set readFds;
			struct timeval timeout = {tls_read_timeout_.tv_sec, tls_read_timeout_.tv_usec};

			do {
				cur_read_len = SSL_read(p_ssl_handle_, &buf[total_read_length], (int) remaining_bytes_to_read);
				if(0 < cur_read_len) {
					total_read_length += (size_t) cur_read_len;
					remaining_bytes_to_read -= cur_read_len;
				} else {
					ssl_retcode = SSL_get_error(p_ssl_handle_, cur_read_len);
					switch(ssl_retcode) {
						case SSL_ERROR_WANT_READ:
							FD_ZERO(&readFds);
							FD_SET(server_tcp_socket_fd_, &readFds);
							select_retCode = select(server_tcp_socket_fd_ + 1, &readFds, NULL, NULL, &timeout);
							if(0 < select_retCode) {
								continue;
							} else if(0 == select_retCode) { //0 == SELECT_TIMEOUT
								errorStatus = ResponseCode::NETWORK_SSL_NOTHING_TO_READ;
							} else { // SELECT_ERROR
								errorStatus = ResponseCode::NETWORK_SSL_READ_ERROR;
							}
							break;
						case SSL_ERROR_ZERO_RETURN:
							errorStatus = ResponseCode::NETWORK_SSL_CONNECTION_CLOSED_ERROR;
							break;
						default:
							errorStatus = ResponseCode::NETWORK_SSL_READ_ERROR;
							break;
					}
				}
				if(ResponseCode::NETWORK_SSL_NOTHING_TO_READ == errorStatus ||
				   ResponseCode::NETWORK_SSL_READ_ERROR == errorStatus ||
				   ResponseCode::NETWORK_SSL_CONNECTION_CLOSED_ERROR == errorStatus) {
					break;
				}
			} while(is_connected_ && total_read_length < size_bytes_to_read);

			if(ResponseCode::SUCCESS == errorStatus) {
				size_read_bytes_out = total_read_length;
			}

			return errorStatus;
		}

		ResponseCode OpenSSLConnection::DisconnectInternal() {
			is_connected_ = false;
			SSL_shutdown(p_ssl_handle_);
#ifdef WIN32
			closesocket(server_tcp_socket_fd_);
#else
			close(server_tcp_socket_fd_);
#endif
			return ResponseCode::SUCCESS;
		}

		OpenSSLConnection::~OpenSSLConnection() {
			if(is_connected_) {
				Disconnect();
			}
			SSL_free(p_ssl_handle_);
			SSL_CTX_free(p_ssl_context_);
#ifdef WIN32
			WSACleanup();
#endif
		}
	}
}
