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
 * @file ResponseCode.hpp
 * @brief Strongly typed enumeration of return values from functions within the SDK.
 *
 * Contains the return codes used by the SDK
 */

#pragma once

// Used to avoid warnings in case of unused parameters in function pointers
#define IOT_UNUSED(x) (void)(x)

namespace awsiotsdk {
	/**
	 * @brief Response Code enum class
	 *
	 * Strongly typed enumeration of return values from functions within the SDK.
	 * Values less than -1 are specific error codes
	 * Value of -1 is a generic failure response
	 * Value of 0 is a generic success response
	 * Values greater than 0 are specific non-error return codes
	 * Values have been grouped based on source or type of code
	 */
	enum class ResponseCode {
		// Shadow Success Codes

		SHADOW_RECEIVED_DELTA = 301,	///< Returned when a delta update is received
		SHADOW_REQUEST_ACCEPTED = 300,	///< Returned when the request has been accepted

		// Network Success Codes

		NETWORK_PHYSICAL_LAYER_CONNECTED = 203,    ///< Returned when the Network physical layer is connected.
		NETWORK_MANUALLY_DISCONNECTED = 202,        ///< Returned when the Network is manually disconnected.
		NETWORK_ATTEMPTING_RECONNECT = 201,        ///< Returned when the Network is disconnected and the reconnect attempt is in progress.
		NETWORK_RECONNECTED = 200,                ///< Return value of yield function to indicate auto-reconnect was successful.

		// MQTT Success Codes

		MQTT_NOTHING_TO_READ = 101,                ///< Returned when a read attempt is made on the TLS buffer and it is empty.
		MQTT_CONNACK_CONNECTION_ACCEPTED = 100,    ///< Returned when a connection request is successful and packet response is connection accepted.

		// Generic Response Codes

		SUCCESS = 0,            ///< Success return value - no error occurred.
		FAILURE = -1,            ///< A generic error. Not enough information for a specific error code.
		NULL_VALUE_ERROR = -2,    ///< A required parameter was passed as null.

		// I/O Error Codes

		FILE_OPEN_ERROR = -100,        ///< Unable to open the requested file

		// Threading Error Codes

		MUTEX_INIT_ERROR = -200,        ///< Mutex initialization failed
		MUTEX_LOCK_ERROR = -201,        ///< Mutex lock request failed
		MUTEX_UNLOCK_ERROR = -202,        ///< Mutex unlock request failed
		MUTEX_DESTROY_ERROR = -203,        ///< Mutex destroy failed
		THREAD_EXITING = -204,				///< Thread is exiting, returned when thread exits in the middle of an operation

		// Network TCP Error Codes

		NETWORK_TCP_CONNECT_ERROR = -300,    ///< The TCP socket could not be established.
		NETWORK_TCP_SETUP_ERROR = -301,        ///< Error associated with setting up the parameters of a Socket.
		NETWORK_TCP_UNKNOWN_HOST = -302,    ///< Returned when the server is unknown.
		NETWORK_TCP_NO_ENDPOINT_SPECIFIED = -303,	///< Returned when the Network connection was not provided an endpoint

		// Network SSL Error Codes

		NETWORK_SSL_INIT_ERROR = -400,                ///< SSL initialization error at the TLS layer.
		NETWORK_SSL_ROOT_CRT_PARSE_ERROR = -401,    ///< Returned when the root certificate is invalid.
		NETWORK_SSL_DEVICE_CRT_PARSE_ERROR = -402,    ///< Returned when the device certificate is invalid.
		NETWORK_SSL_KEY_PARSE_ERROR = -403,            ///< An error occurred when loading the certificates. The certificates could not be located or are incorrectly formatted.
		NETWORK_SSL_TLS_HANDSHAKE_ERROR = -404,        ///< The TLS handshake failed due to unknown error.
		NETWORK_SSL_CONNECT_ERROR = -405,            ///< An unknown occurred while waiting for the TLS handshake to complete.
		NETWORK_SSL_CONNECT_TIMEOUT_ERROR = -406,    ///< A timeout occurred while waiting for the TLS handshake to complete.
		NETWORK_SSL_CONNECTION_CLOSED_ERROR = -407,    ///< The SSL Connection was closed
		NETWORK_SSL_WRITE_ERROR = -408,                ///< A Generic write error based on the platform used.
		NETWORK_SSL_WRITE_TIMEOUT_ERROR = -409,        ///< SSL Write times out.
		NETWORK_SSL_READ_ERROR = -410,                ///< A Generic error based on the platform used.nerator seeding failed.
		NETWORK_SSL_READ_TIMEOUT_ERROR = -411,        ///< SSL Read times out.
		NETWORK_SSL_NOTHING_TO_READ = -412,            ///< Returned when there is nothing to read in the TLS read buffer.
		NETWORK_SSL_UNKNOWN_ERROR = -413,            ///< A generic error code for Network SSL layer errors.
		NETWORK_SSL_SERVER_VERIFICATION_ERROR = -414,	///< Server name verification failure.

		// Network Generic Error Codes

		NETWORK_DISCONNECTED_ERROR = -500,            ///< Returned when the Network is disconnected and reconnect is either disabled or physical layer is disconnected.
		NETWORK_RECONNECT_TIMED_OUT_ERROR = -501,    ///< Returned when the Network is disconnected and the reconnect attempt has timed out.
		NETWORK_ALREADY_CONNECTED_ERROR = -502,        ///< Returned when the Network is already connected and a connection attempt is made.
		NETWORK_PHYSICAL_LAYER_DISCONNECTED = -503,    ///< Returned when the physical layer is disconnected.
		NETWORK_NOTHING_TO_WRITE_ERROR = -504,			///< Returned when the Network write function is passed an empty buffer as argument

		// ClientCore Error Codes

		ACTION_NOT_REGISTERED_ERROR = -601,            ///< Requested action is not registered with the core client
		ACTION_QUEUE_FULL = -602,                    ///< Core Client Action queue is full
		ACTION_CREATE_FAILED = -603,				///< Core Client was not able to create the requested action

		// MQTT Error Codes

		MQTT_CONNECTION_ERROR = -701,                        ///< A connection could not be established.
		MQTT_CONNECT_TIMEOUT_ERROR = -702,                    ///< A timeout occurred while waiting for the MQTT connect to complete.
		MQTT_REQUEST_TIMEOUT_ERROR = -703,                    ///< A timeout occurred while waiting for the TLS request to complete.
		MQTT_UNEXPECTED_CLIENT_STATE_ERROR = -704,            ///< The current client state does not match the expected value.
		MQTT_CLIENT_NOT_IDLE_ERROR = -705,                    ///< The client state is not idle when request is being made.
		MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR = -706,    ///< The MQTT RX buffer received corrupt or unexpected message.
		MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR = -707,        ///< The client is subscribed to the maximum possible number of subscriptions.
		MQTT_DECODE_REMAINING_LENGTH_ERROR = -708,            ///< Failed to decode the remaining packet length on incoming packet.
		MQTT_CONNACK_UNKNOWN_ERROR = -709,                    ///< Connect request failed with the server returning an unknown error.
		MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR = -710,    ///< Connect request failed with the server returning an unacceptable protocol version error.
		MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR = -711,        ///< Connect request failed with the server returning an identifier rejected error.
		MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR = -712,        ///< Connect request failed with the server returning an unavailable error.
		MQTT_CONNACK_BAD_USERDATA_ERROR = -713,                ///< Connect request failed with the server returning a bad userdata error.
		MQTT_CONNACK_NOT_AUTHORIZED_ERROR = -714,            ///< Connect request failed with the server failing to authenticate the request.
		MQTT_NO_SUBSCRIPTION_FOUND = -715,					///< No subscription exists for requested topic
		MQTT_SUBSCRIPTION_NOT_ACTIVE = -716,				///< Subscription exists but is not active, waiting for Suback or Ack not received
		MQTT_UNEXPECTED_PACKET_FORMAT_ERROR = -715,			///< Deserialization failed because packet data was in an unexpected format
		MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST = -716,		///< Too many subscriptions were provided in the Subscribe/Unsubscribe request
		MQTT_INVALID_DATA_ERROR = -717,						///< Provided data is invalid/not sufficient for the request
		MQTT_SUBSCRIBE_PARTIALLY_FAILED = -718,				///< Failed to subscribe to atleast one of the topics in the subscribe request
		MQTT_SUBSCRIBE_FAILED = -719,						///< Unable to subscribe to any of the topics in the subscribe request

		// JSON Parsing Error Codes

		JSON_PARSE_KEY_NOT_FOUND_ERROR = -800,			///< JSON Parser was not able to find the requested key in the specified JSON
		JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR = -801,	///< The value type was different from the expected type
		JSON_PARSING_ERROR = -802,						///< An error occurred while parsing the JSON string.  Usually malformed JSON.
		JSON_MERGE_FAILED = -803,						///< Returned when the JSON merge request fails unexpectedly
		JSON_DIFF_FAILED = -804,						///< Returned when the JSON diff request fails unexpectedly

		// Shadow Error Codes

		SHADOW_WAIT_FOR_PUBLISH = -900,				///< Shadow: The response Ack table is currently full waiting for previously published updates
		SHADOW_JSON_BUFFER_TRUNCATED = -901,		///< Any time an snprintf writes more than size value, this error will be returned
		SHADOW_JSON_ERROR = -902,					///< Any time an snprintf encounters an encoding error or not enough space in the given buffer
		SHADOW_JSON_EMPTY_ERROR = -903,				///< Returned when the provided json document is empty
		SHADOW_REQUEST_MAP_EMPTY = -904,			///< Returned when the provided request map is empty
		SHADOW_MQTT_DISCONNECTED_ERROR = -905,		///< Returned when the MQTT connection is not active
		SHADOW_UNEXPECTED_RESPONSE_TYPE = -906,		///< Returned when the Response type in the recevied payload is unexpected
		SHADOW_UNEXPECTED_RESPONSE_TOPIC = -907,	///< Returned when Response is received on an unexpected topic
		SHADOW_REQUEST_REJECTED = -908,				///< Returned when the request has been rejected by the server
		SHADOW_MQTT_CLIENT_NOT_SET_ERROR = -909,	///< Returned when there is no client set for this shadow
		SHADOW_NOTHING_TO_UPDATE = -910,			///< Returned when there is nothing to update for a Shadow Update request
		SHADOW_UNEXPECTED_RESPONSE_PAYLOAD = -911,	///< Returned when the response payload is in an unexpected format
		SHADOW_RECEIVED_OLD_VERSION_UPDATE = -912,	///< Returned when a version update is received with an older version than the current one on the device

		// WebSocket Error Codes

		WEBSOCKET_SIGN_URL_NO_MEM = -1000,			///< Internal buffer overflow when signing secured WebSocket url
		WEBSOCKET_GEN_CLIENT_KEY_ERROR = -1001,		///< Error in generating WebSocket handhshake client key
		WEBSOCKET_HANDSHAKE_ERROR = -1002,			///< WebSocket handshake generic error
		WEBSOCKET_HANDSHAKE_WRITE = -1003,			///< WebSocket handshake error in sending request
		WEBSOCKET_HANDSHAKE_READ = -1004,			///< WebSocket handhshake error in receiving request
		WEBSOCKET_HANDSHAKE_VERIFY_ERROR = -1005,	///< WebSocket handshake error in verifying server response
		WEBSOCKET_WSLAY_CONTEXT_INIT_ERROR = -1006,	///< WebSocket wslay context init error
		WEBSOCKET_FRAME_RECEIVE_ERROR = -1007,		///< WebSocket error in receiving frames
		WEBSOCKET_FRAME_TRANSMIT_ERROR = -1008,		///< WebSocket error in sending frames
		WEBSOCKET_PROTOCOL_VIOLATION = -1009,		///< WebSocket protocol violation detected in receiving frames
		WEBSOCKET_MAX_LIFETIME_REACHED = -1010,		///< WebSocket connection max life time window reached
		WEBSOCKET_DISCONNECT_ERROR = -1011,			///< WebSocket disconnect error
		WEBSOCKET_GET_UTC_TIME_FAILED = -1012		///< Returned when the WebSocket wrapper cannot get UTC time
	};
}
