# /*
# * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# *
# * Licensed under the Apache License, Version 2.0 (the "License").
# * You may not use this file except in compliance with the License.
# * A copy of the License is located at
# *
# *  http://aws.amazon.com/apache2.0
# *
# * or in the "license" file accompanying this file. This file is distributed
# * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# * express or implied. See the License for the specific language governing
# * permissions and limitations under the License.
# */

import ssl
import logging
from threading import Lock
from numbers import Number
import AWSIoTPythonSDK.core.protocol.paho.client as mqtt
from AWSIoTPythonSDK.core.protocol.paho.client import MQTT_ERR_SUCCESS
from AWSIoTPythonSDK.core.protocol.internal.events import FixedEventMids


class ClientStatus(object):

    IDLE = 0
    CONNECT = 1
    RESUBSCRIBE = 2
    DRAINING = 3
    STABLE = 4
    USER_DISCONNECT = 5
    ABNORMAL_DISCONNECT = 6


class ClientStatusContainer(object):

    def __init__(self):
        self._status = ClientStatus.IDLE

    def get_status(self):
        return self._status

    def set_status(self, status):
        if ClientStatus.USER_DISCONNECT == self._status:  # If user requests to disconnect, no status updates other than user connect
            if ClientStatus.CONNECT == status:
                self._status = status
        else:
            self._status = status


class InternalAsyncMqttClient(object):

    _logger = logging.getLogger(__name__)

    def __init__(self, client_id, clean_session, protocol, use_wss):
        self._paho_client = self._create_paho_client(client_id, clean_session, None, protocol, use_wss)
        self._use_wss = use_wss
        self._event_callback_map_lock = Lock()
        self._event_callback_map = dict()

    def _create_paho_client(self, client_id, clean_session, user_data, protocol, use_wss):
        self._logger.debug("Initializing MQTT layer...")
        return mqtt.Client(client_id, clean_session, user_data, protocol, use_wss)

    #  TODO: Merge credentials providers configuration into one
    def set_cert_credentials_provider(self, cert_credentials_provider):
        # History issue from Yun SDK where AR9331 embedded Linux only have Python 2.7.3
        # pre-installed. In this version, TLSv1_2 is not even an option.
        # SSLv23 is a work-around which selects the highest TLS version between the client
        # and service. If user installs opensslv1.0.1+, this option will work fine for Mutual
        # Auth.
        # Note that we cannot force TLSv1.2 for Mutual Auth. in Python 2.7.3 and TLS support
        # in Python only starts from Python2.7.
        # See also: https://docs.python.org/2/library/ssl.html#ssl.PROTOCOL_SSLv23
        if self._use_wss:
            ca_path = cert_credentials_provider.get_ca_path()
            self._paho_client.tls_set(ca_certs=ca_path, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_SSLv23)
        else:
            ca_path = cert_credentials_provider.get_ca_path()
            cert_path = cert_credentials_provider.get_cert_path()
            key_path = cert_credentials_provider.get_key_path()
            self._paho_client.tls_set(ca_certs=ca_path,certfile=cert_path, keyfile=key_path,
                                      cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_SSLv23)

    def set_iam_credentials_provider(self, iam_credentials_provider):
        self._paho_client.configIAMCredentials(iam_credentials_provider.get_access_key_id(),
                                               iam_credentials_provider.get_secret_access_key(),
                                               iam_credentials_provider.get_session_token())

    def set_endpoint_provider(self, endpoint_provider):
        self._endpoint_provider = endpoint_provider

    def configure_last_will(self, topic, payload, qos, retain=False):
        self._paho_client.will_set(topic, payload, qos, retain)

    def configure_alpn_protocols(self, alpn_protocols):
        self._paho_client.config_alpn_protocols(alpn_protocols)

    def clear_last_will(self):
        self._paho_client.will_clear()

    def set_username_password(self, username, password=None):
        self._paho_client.username_pw_set(username, password)

    def set_socket_factory(self, socket_factory):
        self._paho_client.socket_factory_set(socket_factory)
        
    def configure_reconnect_back_off(self, base_reconnect_quiet_sec, max_reconnect_quiet_sec, stable_connection_sec):
        self._paho_client.setBackoffTiming(base_reconnect_quiet_sec, max_reconnect_quiet_sec, stable_connection_sec)

    def connect(self, keep_alive_sec, ack_callback=None):
        host = self._endpoint_provider.get_host()
        port = self._endpoint_provider.get_port()

        with self._event_callback_map_lock:
            self._logger.debug("Filling in fixed event callbacks: CONNACK, DISCONNECT, MESSAGE")
            self._event_callback_map[FixedEventMids.CONNACK_MID] = self._create_combined_on_connect_callback(ack_callback)
            self._event_callback_map[FixedEventMids.DISCONNECT_MID] = self._create_combined_on_disconnect_callback(None)
            self._event_callback_map[FixedEventMids.MESSAGE_MID] = self._create_converted_on_message_callback()

            rc = self._paho_client.connect(host, port, keep_alive_sec)
            if MQTT_ERR_SUCCESS == rc:
                self.start_background_network_io()

            return rc

    def start_background_network_io(self):
        self._logger.debug("Starting network I/O thread...")
        self._paho_client.loop_start()

    def stop_background_network_io(self):
        self._logger.debug("Stopping network I/O thread...")
        self._paho_client.loop_stop()

    def disconnect(self, ack_callback=None):
        with self._event_callback_map_lock:
            rc = self._paho_client.disconnect()
            if MQTT_ERR_SUCCESS == rc:
                self._logger.debug("Filling in custom disconnect event callback...")
                combined_on_disconnect_callback = self._create_combined_on_disconnect_callback(ack_callback)
                self._event_callback_map[FixedEventMids.DISCONNECT_MID] = combined_on_disconnect_callback
            return rc

    def _create_combined_on_connect_callback(self, ack_callback):
        def combined_on_connect_callback(mid, data):
            self.on_online()
            if ack_callback:
                ack_callback(mid, data)
        return combined_on_connect_callback

    def _create_combined_on_disconnect_callback(self, ack_callback):
        def combined_on_disconnect_callback(mid, data):
            self.on_offline()
            if ack_callback:
                ack_callback(mid, data)
        return combined_on_disconnect_callback

    def _create_converted_on_message_callback(self):
        def converted_on_message_callback(mid, data):
            self.on_message(data)
        return converted_on_message_callback

    # For client online notification
    def on_online(self):
        pass

    # For client offline notification
    def on_offline(self):
        pass

    # For client message reception notification
    def on_message(self, message):
        pass

    def publish(self, topic, payload, qos, retain=False, ack_callback=None):
        with self._event_callback_map_lock:
            rc, mid = self._paho_client.publish(topic, payload, qos, retain)
            if MQTT_ERR_SUCCESS == rc and qos > 0 and ack_callback:
                self._logger.debug("Filling in custom puback (QoS>0) event callback...")
                self._event_callback_map[mid] = ack_callback
            return rc, mid

    def subscribe(self, topic, qos, ack_callback=None):
        with self._event_callback_map_lock:
            rc, mid = self._paho_client.subscribe(topic, qos)
            if MQTT_ERR_SUCCESS == rc and ack_callback:
                self._logger.debug("Filling in custom suback event callback...")
                self._event_callback_map[mid] = ack_callback
            return rc, mid

    def unsubscribe(self, topic, ack_callback=None):
        with self._event_callback_map_lock:
            rc, mid = self._paho_client.unsubscribe(topic)
            if MQTT_ERR_SUCCESS == rc and ack_callback:
                self._logger.debug("Filling in custom unsuback event callback...")
                self._event_callback_map[mid] = ack_callback
            return rc, mid

    def register_internal_event_callbacks(self, on_connect, on_disconnect, on_publish, on_subscribe, on_unsubscribe, on_message):
        self._logger.debug("Registering internal event callbacks to MQTT layer...")
        self._paho_client.on_connect = on_connect
        self._paho_client.on_disconnect = on_disconnect
        self._paho_client.on_publish = on_publish
        self._paho_client.on_subscribe = on_subscribe
        self._paho_client.on_unsubscribe = on_unsubscribe
        self._paho_client.on_message = on_message

    def unregister_internal_event_callbacks(self):
        self._logger.debug("Unregistering internal event callbacks from MQTT layer...")
        self._paho_client.on_connect = None
        self._paho_client.on_disconnect = None
        self._paho_client.on_publish = None
        self._paho_client.on_subscribe = None
        self._paho_client.on_unsubscribe = None
        self._paho_client.on_message = None

    def invoke_event_callback(self, mid, data=None):
        with self._event_callback_map_lock:
            event_callback = self._event_callback_map.get(mid)
        # For invoking the event callback, we do not need to acquire the lock
        if event_callback:
            self._logger.debug("Invoking custom event callback...")
            if data is not None:
                event_callback(mid=mid, data=data)
            else:
                event_callback(mid=mid)
            if isinstance(mid, Number):  # Do NOT remove callbacks for CONNACK/DISCONNECT/MESSAGE
                self._logger.debug("This custom event callback is for pub/sub/unsub, removing it after invocation...")
                with self._event_callback_map_lock:
                    del self._event_callback_map[mid]

    def remove_event_callback(self, mid):
        with self._event_callback_map_lock:
            if mid in self._event_callback_map:
                self._logger.debug("Removing custom event callback...")
                del self._event_callback_map[mid]

    def clean_up_event_callbacks(self):
        with self._event_callback_map_lock:
            self._event_callback_map.clear()

    def get_event_callback_map(self):
        return self._event_callback_map
