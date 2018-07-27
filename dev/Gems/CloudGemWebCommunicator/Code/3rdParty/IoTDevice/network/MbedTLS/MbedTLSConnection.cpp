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
 * @brief
 *
 */

#include "StdAfx.h"

#include <iostream>
#include <util/memory/stl/Vector.hpp>

#include "MbedTLSConnection.hpp"
#include "util/logging/LogMacros.hpp"

#define MBEDTLS_WRAPPER_LOG_TAG "[MbedTLS Wrapper]"
#define MAX_CHARS_IN_PORT_NUMBER 6

namespace awsiotsdk {
	namespace network {
		MbedTLSConnection::MbedTLSConnection(util::String endpoint, uint16_t endpoint_port, util::String root_ca_location,
											 util::String device_cert_location, util::String device_private_key_location,
											 std::chrono::milliseconds tls_handshake_timeout,
											 std::chrono::milliseconds tls_read_timeout,
											 std::chrono::milliseconds tls_write_timeout,
											 bool server_verification_flag) {
			endpoint_ = endpoint;
			endpoint_port_ = endpoint_port;
			root_ca_location_ = root_ca_location;
			device_cert_location_ = device_cert_location;
			device_private_key_location_ = device_private_key_location;
			server_verification_flag_ = server_verification_flag;
			tls_handshake_timeout_ = tls_handshake_timeout;
			tls_read_timeout_ = tls_read_timeout;
			tls_write_timeout_ = tls_write_timeout;
			flags_ = 0;

			is_connected_ = false;
		}

		bool MbedTLSConnection::IsPhysicalLayerConnected() {
			// Use this to add implementation which can check for physical layer disconnect
			return true;
		}

		bool MbedTLSConnection::IsConnected() {
			return is_connected_;
		}

		int MbedTLSConnection::VerifyCertificate(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
			char buf[1024];
			((void) data);

			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "Verify requested for (Depth %d):", depth);
			mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "%s", buf);

			if((*flags) == 0) {
				AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "This certificate has no flags");
			} else {
				AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, buf, sizeof(buf), "  ! ", *flags);
				AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "%s\n", buf);
			}

			return 0;
		}

		ResponseCode MbedTLSConnection::ConnectInternal() {
			ResponseCode rc = ResponseCode::SUCCESS;

			int ret = 0;
			const util::String pers = "aws_iot_tls_wrapper";
			char port_buf[6];
			char vrfy_buf[512];

			mbedtls_net_init(&server_fd_);
			mbedtls_ssl_init(&ssl_);
			mbedtls_ssl_config_init(&conf_);
			mbedtls_ctr_drbg_init(&ctr_drbg_);
			mbedtls_x509_crt_init(&cacert_);
			mbedtls_x509_crt_init(&clicert_);
			mbedtls_pk_init(&pkey_);

			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "...............................%d", MBEDTLS_SSL_MAX_CONTENT_LEN);

			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "....Seeding the random number generator...");
			mbedtls_entropy_init(&entropy_);
			if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg_, mbedtls_entropy_func, &entropy_,
											(const unsigned char *)(pers.c_str()), pers.length())) != 0) {
				AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, " Connect Failed!!! mbedtls_ctr_drbg_seed returned -0x%x", -ret);
				return ResponseCode::NETWORK_SSL_INIT_ERROR;
			}

			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "....Loading the CA root certificate...");
			ret = mbedtls_x509_crt_parse_file(&cacert_, root_ca_location_.c_str());
			if(ret < 0) {
				AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!!  mbedtls_x509_crt_parse returned -0x%x while parsing root cert\n\n", -ret);
				return ResponseCode::NETWORK_SSL_ROOT_CRT_PARSE_ERROR;
			}
			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "ok (%d skipped)\n", ret);

			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "....Loading the client cert. and key...");
			ret = mbedtls_x509_crt_parse_file(&clicert_, device_cert_location_.c_str());
			if(ret != 0) {
				AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!!  mbedtls_x509_crt_parse returned -0x%x while parsing device cert\n\n", -ret);
				return ResponseCode::NETWORK_SSL_DEVICE_CRT_PARSE_ERROR;
			}

			ret = mbedtls_pk_parse_keyfile(&pkey_, device_private_key_location_.c_str(), "");
			if(ret != 0) {
				AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!!  mbedtls_pk_parse_key returned -0x%x while parsing private key\n\n", -ret);
				AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, " path : %s ", device_private_key_location_.c_str());
				return ResponseCode::NETWORK_SSL_KEY_PARSE_ERROR;
			}
			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, " ok\n");
			snprintf(port_buf, MAX_CHARS_IN_PORT_NUMBER, "%d", endpoint_port_);
			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "....Connecting to %s/%s...", endpoint_.c_str(), port_buf);
			if((ret = mbedtls_net_connect(&server_fd_, endpoint_.c_str(), port_buf, MBEDTLS_NET_PROTO_TCP)) != 0) {
				AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!! mbedtls_net_connect returned -0x%x\n\n", -ret);
				switch(ret) {
					case MBEDTLS_ERR_NET_SOCKET_FAILED:
						return ResponseCode::NETWORK_TCP_SETUP_ERROR;
					case MBEDTLS_ERR_NET_UNKNOWN_HOST:
						return ResponseCode::NETWORK_TCP_UNKNOWN_HOST;
					case MBEDTLS_ERR_NET_CONNECT_FAILED:
					default:
						return ResponseCode::NETWORK_TCP_CONNECT_ERROR;
				};
			}

			ret = mbedtls_net_set_block(&server_fd_);
			if(ret != 0) {
				AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!! net_set_(non)block() returned -0x%x\n\n", -ret);
				return ResponseCode::NETWORK_SSL_UNKNOWN_ERROR;
			} AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "Ok!");

			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "....Setting up the SSL/TLS structure...");
			if((ret = mbedtls_ssl_config_defaults(&conf_, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
												  MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
				AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!! mbedtls_ssl_config_defaults returned -0x%x\n\n", -ret);
				return ResponseCode::NETWORK_SSL_UNKNOWN_ERROR;
			}

			mbedtls_ssl_conf_verify(&conf_, &MbedTLSConnection::VerifyCertificate, NULL);
			if(server_verification_flag_) {
				mbedtls_ssl_conf_authmode(&conf_, MBEDTLS_SSL_VERIFY_REQUIRED);
			} else {
				mbedtls_ssl_conf_authmode(&conf_, MBEDTLS_SSL_VERIFY_OPTIONAL);
			}
			mbedtls_ssl_conf_rng(&conf_, mbedtls_ctr_drbg_random, &ctr_drbg_);

			mbedtls_ssl_conf_ca_chain(&conf_, &cacert_, NULL);
			if((ret = mbedtls_ssl_conf_own_cert(&conf_, &clicert_, &pkey_)) !=
			   0) {
				AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
				return ResponseCode::NETWORK_SSL_UNKNOWN_ERROR;
			}

			mbedtls_ssl_conf_read_timeout(&conf_, static_cast<uint32_t>(tls_handshake_timeout_.count()));

			if((ret = mbedtls_ssl_setup(&ssl_, &conf_)) != 0) {
				AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!! mbedtls_ssl_setup returned -0x%x\n\n", -ret);
				return ResponseCode::NETWORK_SSL_UNKNOWN_ERROR;
			}
			if((ret = mbedtls_ssl_set_hostname(&ssl_, endpoint_.c_str())) != 0) {
				AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!! mbedtls_ssl_set_hostname returned %d\n\n", ret);
				return ResponseCode::NETWORK_SSL_UNKNOWN_ERROR;
			}
			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "\n\nSSL state connect : %d ", ssl_.state);
			mbedtls_ssl_set_bio(&ssl_, &server_fd_, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);
			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "Ok!");

			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "\n\nSSL state connect : %d ", ssl_.state);
			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "....Performing the SSL/TLS handshake...");
			while((ret = mbedtls_ssl_handshake(&ssl_)) != 0) {
				if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
					AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!! mbedtls_ssl_handshake returned -0x%x\n", -ret);
					if(ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
						AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "    Unable to verify the server's certificate. "
										  "Either it is invalid,\n"
										  "    or you didn't set ca_file or ca_path "
										  "to an appropriate value.\n"
										  "    Alternatively, you may want to use "
										  "auth_mode=optional for testing purposes.\n");
					}
					return ResponseCode::NETWORK_SSL_TLS_HANDSHAKE_ERROR;
				}
			}

			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, " ok\n    [ Protocol is %s ]\n    [ Ciphersuite is %s ]\n", mbedtls_ssl_get_version(&ssl_),
					  mbedtls_ssl_get_ciphersuite(&ssl_));
			if((ret = mbedtls_ssl_get_record_expansion(&ssl_)) >= 0) {
				AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "    [ Record expansion is %d ]\n", ret);
			} else {
				AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "    [ Record expansion is unknown (compression) ]\n");
			}

			AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, "....Verifying peer X.509 certificate...");

			if(server_verification_flag_) {
				if((flags_ = mbedtls_ssl_get_verify_result(&ssl_)) != 0) {
					AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, " failed\n");
					mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags_);
					AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "%s\n", vrfy_buf);
					rc = ResponseCode::NETWORK_SSL_SERVER_VERIFICATION_ERROR;
				} else {
					AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, " ok\n");
					rc = ResponseCode::SUCCESS;
				}
			} else {
				AWS_LOG_INFO(MBEDTLS_WRAPPER_LOG_TAG, " Server Verification skipped\n");
				rc = ResponseCode::SUCCESS;
			}

			mbedtls_ssl_conf_read_timeout(&conf_, static_cast<uint32_t>(tls_read_timeout_.count()));
			is_connected_ = true;
			return rc;
		}

		ResponseCode MbedTLSConnection::WriteInternal(const util::String &buf, size_t &size_written_bytes_out) {
			size_t total_written_length = 0;
			ResponseCode rc = ResponseCode::SUCCESS;
			size_t bytes_to_write = buf.length();
			const unsigned char *buf_cstr = (const unsigned char *)(buf.c_str());
			bool isErrorFlag = false;
			int ret;

			auto timeout = std::chrono::system_clock::now() + tls_write_timeout_;
			auto now = std::chrono::system_clock::now();

			do {
				ret = mbedtls_ssl_write(&ssl_, buf_cstr + total_written_length, bytes_to_write - total_written_length);
				if(ret > 0) {
					total_written_length += ret;
				} else if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
					AWS_LOG_ERROR(MBEDTLS_WRAPPER_LOG_TAG, "Failed!!! mbedtls_ssl_write returned -0x%x\n\n", -ret);
					/* All other negative return values indicate connection needs to be reset.
					 * Will be caught in ping request so ignored here */
					isErrorFlag = true;
					break;
				}
				now = std::chrono::system_clock::now();
			} while(now < timeout && total_written_length < bytes_to_write);

			size_written_bytes_out = total_written_length;

			if(isErrorFlag) {
				rc = ResponseCode::NETWORK_SSL_WRITE_ERROR;
			} else if(now < timeout && total_written_length != bytes_to_write) {
				return ResponseCode::NETWORK_SSL_WRITE_TIMEOUT_ERROR;
			}

			return rc;
		}

		ResponseCode MbedTLSConnection::ReadInternal(util::Vector<unsigned char> &buf, size_t buf_read_offset,
													 size_t size_bytes_to_read, size_t &size_read_bytes_out) {
			int ret;
			size_t total_read_length = 0;
			size_t remaining_bytes_to_read = size_bytes_to_read;
			auto timeout = std::chrono::system_clock::now() + tls_read_timeout_;
			do {
				// This read will timeout after IOT_SSL_READ_TIMEOUT if there's no data to be read
				ret = mbedtls_ssl_read(&ssl_, &buf[buf_read_offset], remaining_bytes_to_read);
				if(ret > 0) {
					buf_read_offset += ret;
					total_read_length += ret;
					remaining_bytes_to_read -= ret;
					size_read_bytes_out = total_read_length;
				} else if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_TIMEOUT) {
					return ResponseCode::NETWORK_SSL_READ_ERROR;
				}
			} while(remaining_bytes_to_read > 0 && timeout < std::chrono::system_clock::now());

			if(0 == total_read_length) {
				return ResponseCode::NETWORK_SSL_NOTHING_TO_READ;
			} else if(size_read_bytes_out == size_bytes_to_read) {
				return ResponseCode::SUCCESS;
			} else if(0 < total_read_length) {
				return ResponseCode::NETWORK_SSL_READ_TIMEOUT_ERROR;
			}

			return ResponseCode::NETWORK_SSL_READ_ERROR;
		}

		ResponseCode MbedTLSConnection::DisconnectInternal() {
			int ret = 0;
			do {
				ret = mbedtls_ssl_close_notify(&ssl_);
			} while(ret == MBEDTLS_ERR_SSL_WANT_WRITE);

			is_connected_ = false;

			/* All other negative return values indicate connection needs to be reset.
			 * No further action required since this is disconnect call */
			return ResponseCode::SUCCESS;
		}

		MbedTLSConnection::~MbedTLSConnection() {
			if(is_connected_) {
				Disconnect();
			}

			mbedtls_net_free(&server_fd_);

			mbedtls_x509_crt_free(&clicert_);
			mbedtls_x509_crt_free(&cacert_);
			mbedtls_pk_free(&pkey_);
			mbedtls_ssl_free(&ssl_);
			mbedtls_ssl_config_free(&conf_);
			mbedtls_ctr_drbg_free(&ctr_drbg_);
			mbedtls_entropy_free(&entropy_);
		}
	}
}
