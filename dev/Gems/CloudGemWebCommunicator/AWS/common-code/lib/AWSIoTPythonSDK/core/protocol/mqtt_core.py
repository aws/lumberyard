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

import AWSIoTPythonSDK
from AWSIoTPythonSDK.core.protocol.internal.clients import InternalAsyncMqttClient
from AWSIoTPythonSDK.core.protocol.internal.clients import ClientStatusContainer
from AWSIoTPythonSDK.core.protocol.internal.clients import ClientStatus
from AWSIoTPythonSDK.core.protocol.internal.workers import EventProducer
from AWSIoTPythonSDK.core.protocol.internal.workers import EventConsumer
from AWSIoTPythonSDK.core.protocol.internal.workers import SubscriptionManager
from AWSIoTPythonSDK.core.protocol.internal.workers import OfflineRequestsManager
from AWSIoTPythonSDK.core.protocol.internal.requests import RequestTypes
from AWSIoTPythonSDK.core.protocol.internal.requests import QueueableRequest
from AWSIoTPythonSDK.core.protocol.internal.defaults import DEFAULT_CONNECT_DISCONNECT_TIMEOUT_SEC
from AWSIoTPythonSDK.core.protocol.internal.defaults import DEFAULT_OPERATION_TIMEOUT_SEC
from AWSIoTPythonSDK.core.protocol.internal.defaults import METRICS_PREFIX
from AWSIoTPythonSDK.core.protocol.internal.defaults import ALPN_PROTCOLS
from AWSIoTPythonSDK.core.protocol.internal.events import FixedEventMids
from AWSIoTPythonSDK.core.protocol.paho.client import MQTT_ERR_SUCCESS
from AWSIoTPythonSDK.exception.AWSIoTExceptions import connectError
from AWSIoTPythonSDK.exception.AWSIoTExceptions import connectTimeoutException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import disconnectError
from AWSIoTPythonSDK.exception.AWSIoTExceptions import disconnectTimeoutException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import publishError
from AWSIoTPythonSDK.exception.AWSIoTExceptions import publishTimeoutException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import publishQueueFullException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import publishQueueDisabledException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import subscribeQueueFullException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import subscribeQueueDisabledException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import unsubscribeQueueFullException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import unsubscribeQueueDisabledException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import subscribeError
from AWSIoTPythonSDK.exception.AWSIoTExceptions import subscribeTimeoutException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import unsubscribeError
from AWSIoTPythonSDK.exception.AWSIoTExceptions import unsubscribeTimeoutException
from AWSIoTPythonSDK.core.protocol.internal.queues import AppendResults
from AWSIoTPythonSDK.core.util.enums import DropBehaviorTypes
from AWSIoTPythonSDK.core.protocol.paho.client import MQTTv31
from threading import Condition
from threading import Event
import logging
import sys
if sys.version_info[0] < 3:
    from Queue import Queue
else:
    from queue import Queue


class MqttCore(object):

    _logger = logging.getLogger(__name__)

    def __init__(self, client_id, clean_session, protocol, use_wss):
        self._use_wss = use_wss
        self._username = ""
        self._password = None
        self._enable_metrics_collection = True
        self._event_queue = Queue()
        self._event_cv = Condition()
        self._event_producer = EventProducer(self._event_cv, self._event_queue)
        self._client_status = ClientStatusContainer()
        self._internal_async_client = InternalAsyncMqttClient(client_id, clean_session, protocol, use_wss)
        self._subscription_manager = SubscriptionManager()
        self._offline_requests_manager = OfflineRequestsManager(-1, DropBehaviorTypes.DROP_NEWEST)  # Infinite queue
        self._event_consumer = EventConsumer(self._event_cv,
                                             self._event_queue,
                                             self._internal_async_client,
                                             self._subscription_manager,
                                             self._offline_requests_manager,
                                             self._client_status)
        self._connect_disconnect_timeout_sec = DEFAULT_CONNECT_DISCONNECT_TIMEOUT_SEC
        self._operation_timeout_sec = DEFAULT_OPERATION_TIMEOUT_SEC
        self._init_offline_request_exceptions()
        self._init_workers()
        self._logger.info("MqttCore initialized")
        self._logger.info("Client id: %s" % client_id)
        self._logger.info("Protocol version: %s" % ("MQTTv3.1" if protocol == MQTTv31 else "MQTTv3.1.1"))
        self._logger.info("Authentication type: %s" % ("SigV4 WebSocket" if use_wss else "TLSv1.2 certificate based Mutual Auth."))

    def _init_offline_request_exceptions(self):
        self._offline_request_queue_disabled_exceptions = {
            RequestTypes.PUBLISH : publishQueueDisabledException(),
            RequestTypes.SUBSCRIBE : subscribeQueueDisabledException(),
            RequestTypes.UNSUBSCRIBE : unsubscribeQueueDisabledException()
        }
        self._offline_request_queue_full_exceptions = {
            RequestTypes.PUBLISH : publishQueueFullException(),
            RequestTypes.SUBSCRIBE : subscribeQueueFullException(),
            RequestTypes.UNSUBSCRIBE : unsubscribeQueueFullException()
        }

    def _init_workers(self):
        self._internal_async_client.register_internal_event_callbacks(self._event_producer.on_connect,
                                                                      self._event_producer.on_disconnect,
                                                                      self._event_producer.on_publish,
                                                                      self._event_producer.on_subscribe,
                                                                      self._event_producer.on_unsubscribe,
                                                                      self._event_producer.on_message)

    def _start_workers(self):
        self._event_consumer.start()

    def use_wss(self):
        return self._use_wss

    # Used for general message event reception
    def on_message(self, message):
        pass

    # Used for general online event notification
    def on_online(self):
        pass

    # Used for general offline event notification
    def on_offline(self):
        pass

    def configure_cert_credentials(self, cert_credentials_provider):
        self._logger.info("Configuring certificates...")
        self._internal_async_client.set_cert_credentials_provider(cert_credentials_provider)

    def configure_iam_credentials(self, iam_credentials_provider):
        self._logger.info("Configuring custom IAM credentials...")
        self._internal_async_client.set_iam_credentials_provider(iam_credentials_provider)

    def configure_endpoint(self, endpoint_provider):
        self._logger.info("Configuring endpoint...")
        self._internal_async_client.set_endpoint_provider(endpoint_provider)

    def configure_connect_disconnect_timeout_sec(self, connect_disconnect_timeout_sec):
        self._logger.info("Configuring connect/disconnect time out: %f sec" % connect_disconnect_timeout_sec)
        self._connect_disconnect_timeout_sec = connect_disconnect_timeout_sec

    def configure_operation_timeout_sec(self, operation_timeout_sec):
        self._logger.info("Configuring MQTT operation time out: %f sec" % operation_timeout_sec)
        self._operation_timeout_sec = operation_timeout_sec

    def configure_reconnect_back_off(self, base_reconnect_quiet_sec, max_reconnect_quiet_sec, stable_connection_sec):
        self._logger.info("Configuring reconnect back off timing...")
        self._logger.info("Base quiet time: %f sec" % base_reconnect_quiet_sec)
        self._logger.info("Max quiet time: %f sec" % max_reconnect_quiet_sec)
        self._logger.info("Stable connection time: %f sec" % stable_connection_sec)
        self._internal_async_client.configure_reconnect_back_off(base_reconnect_quiet_sec, max_reconnect_quiet_sec, stable_connection_sec)

    def configure_alpn_protocols(self):
        self._logger.info("Configuring alpn protocols...")
        self._internal_async_client.configure_alpn_protocols([ALPN_PROTCOLS])

    def configure_last_will(self, topic, payload, qos, retain=False):
        self._logger.info("Configuring last will...")
        self._internal_async_client.configure_last_will(topic, payload, qos, retain)

    def clear_last_will(self):
        self._logger.info("Clearing last will...")
        self._internal_async_client.clear_last_will()

    def configure_username_password(self, username, password=None):
        self._logger.info("Configuring username and password...")
        self._username = username
        self._password = password

    def configure_socket_factory(self, socket_factory):
        self._logger.info("Configuring socket factory...")
        self._internal_async_client.set_socket_factory(socket_factory)
        
    def enable_metrics_collection(self):
        self._enable_metrics_collection = True

    def disable_metrics_collection(self):
        self._enable_metrics_collection = False

    def configure_offline_requests_queue(self, max_size, drop_behavior):
        self._logger.info("Configuring offline requests queueing: max queue size: %d", max_size)
        self._offline_requests_manager = OfflineRequestsManager(max_size, drop_behavior)
        self._event_consumer.update_offline_requests_manager(self._offline_requests_manager)

    def configure_draining_interval_sec(self, draining_interval_sec):
        self._logger.info("Configuring offline requests queue draining interval: %f sec", draining_interval_sec)
        self._event_consumer.update_draining_interval_sec(draining_interval_sec)

    def connect(self, keep_alive_sec):
        self._logger.info("Performing sync connect...")
        event = Event()
        self.connect_async(keep_alive_sec, self._create_blocking_ack_callback(event))
        if not event.wait(self._connect_disconnect_timeout_sec):
            self._logger.error("Connect timed out")
            raise connectTimeoutException()
        return True

    def connect_async(self, keep_alive_sec, ack_callback=None):
        self._logger.info("Performing async connect...")
        self._logger.info("Keep-alive: %f sec" % keep_alive_sec)
        self._start_workers()
        self._load_callbacks()
        self._load_username_password()

        try:
            self._client_status.set_status(ClientStatus.CONNECT)
            rc = self._internal_async_client.connect(keep_alive_sec, ack_callback)
            if MQTT_ERR_SUCCESS != rc:
                self._logger.error("Connect error: %d", rc)
                raise connectError(rc)
        except Exception as e:
            # Provided any error in connect, we should clean up the threads that have been created
            self._event_consumer.stop()
            if not self._event_consumer.wait_until_it_stops(self._connect_disconnect_timeout_sec):
                self._logger.error("Time out in waiting for event consumer to stop")
            else:
                self._logger.debug("Event consumer stopped")
            self._client_status.set_status(ClientStatus.IDLE)
            raise e

        return FixedEventMids.CONNACK_MID

    def _load_callbacks(self):
        self._logger.debug("Passing in general notification callbacks to internal client...")
        self._internal_async_client.on_online = self.on_online
        self._internal_async_client.on_offline = self.on_offline
        self._internal_async_client.on_message = self.on_message

    def _load_username_password(self):
        username_candidate = self._username
        if self._enable_metrics_collection:
            username_candidate += METRICS_PREFIX
            username_candidate += AWSIoTPythonSDK.__version__
        self._internal_async_client.set_username_password(username_candidate, self._password)

    def disconnect(self):
        self._logger.info("Performing sync disconnect...")
        event = Event()
        self.disconnect_async(self._create_blocking_ack_callback(event))
        if not event.wait(self._connect_disconnect_timeout_sec):
            self._logger.error("Disconnect timed out")
            raise disconnectTimeoutException()
        if not self._event_consumer.wait_until_it_stops(self._connect_disconnect_timeout_sec):
            self._logger.error("Disconnect timed out in waiting for event consumer")
            raise disconnectTimeoutException()
        return True

    def disconnect_async(self, ack_callback=None):
        self._logger.info("Performing async disconnect...")
        self._client_status.set_status(ClientStatus.USER_DISCONNECT)
        rc = self._internal_async_client.disconnect(ack_callback)
        if MQTT_ERR_SUCCESS != rc:
            self._logger.error("Disconnect error: %d", rc)
            raise disconnectError(rc)
        return FixedEventMids.DISCONNECT_MID

    def publish(self, topic, payload, qos, retain=False):
        self._logger.info("Performing sync publish...")
        ret = False
        if ClientStatus.STABLE != self._client_status.get_status():
            self._handle_offline_request(RequestTypes.PUBLISH, (topic, payload, qos, retain))
        else:
            if qos > 0:
                event = Event()
                rc, mid = self._publish_async(topic, payload, qos, retain, self._create_blocking_ack_callback(event))
                if not event.wait(self._operation_timeout_sec):
                    self._internal_async_client.remove_event_callback(mid)
                    self._logger.error("Publish timed out")
                    raise publishTimeoutException()
            else:
                self._publish_async(topic, payload, qos, retain)
            ret = True
        return ret

    def publish_async(self, topic, payload, qos, retain=False, ack_callback=None):
        self._logger.info("Performing async publish...")
        if ClientStatus.STABLE != self._client_status.get_status():
            self._handle_offline_request(RequestTypes.PUBLISH, (topic, payload, qos, retain))
            return FixedEventMids.QUEUED_MID
        else:
            rc, mid = self._publish_async(topic, payload, qos, retain, ack_callback)
            return mid

    def _publish_async(self, topic, payload, qos, retain=False, ack_callback=None):
        rc, mid = self._internal_async_client.publish(topic, payload, qos, retain, ack_callback)
        if MQTT_ERR_SUCCESS != rc:
            self._logger.error("Publish error: %d", rc)
            raise publishError(rc)
        return rc, mid

    def subscribe(self, topic, qos, message_callback=None):
        self._logger.info("Performing sync subscribe...")
        ret = False
        if ClientStatus.STABLE != self._client_status.get_status():
            self._handle_offline_request(RequestTypes.SUBSCRIBE, (topic, qos, message_callback, None))
        else:
            event = Event()
            rc, mid = self._subscribe_async(topic, qos, self._create_blocking_ack_callback(event), message_callback)
            if not event.wait(self._operation_timeout_sec):
                self._internal_async_client.remove_event_callback(mid)
                self._logger.error("Subscribe timed out")
                raise subscribeTimeoutException()
            ret = True
        return ret

    def subscribe_async(self, topic, qos, ack_callback=None, message_callback=None):
        self._logger.info("Performing async subscribe...")
        if ClientStatus.STABLE != self._client_status.get_status():
            self._handle_offline_request(RequestTypes.SUBSCRIBE, (topic, qos, message_callback, ack_callback))
            return FixedEventMids.QUEUED_MID
        else:
            rc, mid = self._subscribe_async(topic, qos, ack_callback, message_callback)
            return mid

    def _subscribe_async(self, topic, qos, ack_callback=None, message_callback=None):
        self._subscription_manager.add_record(topic, qos, message_callback, ack_callback)
        rc, mid = self._internal_async_client.subscribe(topic, qos, ack_callback)
        if MQTT_ERR_SUCCESS != rc:
            self._logger.error("Subscribe error: %d", rc)
            raise subscribeError(rc)
        return rc, mid

    def unsubscribe(self, topic):
        self._logger.info("Performing sync unsubscribe...")
        ret = False
        if ClientStatus.STABLE != self._client_status.get_status():
            self._handle_offline_request(RequestTypes.UNSUBSCRIBE, (topic, None))
        else:
            event = Event()
            rc, mid = self._unsubscribe_async(topic, self._create_blocking_ack_callback(event))
            if not event.wait(self._operation_timeout_sec):
                self._internal_async_client.remove_event_callback(mid)
                self._logger.error("Unsubscribe timed out")
                raise unsubscribeTimeoutException()
            ret = True
        return ret

    def unsubscribe_async(self, topic, ack_callback=None):
        self._logger.info("Performing async unsubscribe...")
        if ClientStatus.STABLE != self._client_status.get_status():
            self._handle_offline_request(RequestTypes.UNSUBSCRIBE, (topic, ack_callback))
            return FixedEventMids.QUEUED_MID
        else:
            rc, mid = self._unsubscribe_async(topic, ack_callback)
            return mid

    def _unsubscribe_async(self, topic, ack_callback=None):
        self._subscription_manager.remove_record(topic)
        rc, mid = self._internal_async_client.unsubscribe(topic, ack_callback)
        if MQTT_ERR_SUCCESS != rc:
            self._logger.error("Unsubscribe error: %d", rc)
            raise unsubscribeError(rc)
        return rc, mid

    def _create_blocking_ack_callback(self, event):
        def ack_callback(mid, data=None):
            event.set()
        return ack_callback

    def _handle_offline_request(self, type, data):
        self._logger.info("Offline request detected!")
        offline_request = QueueableRequest(type, data)
        append_result = self._offline_requests_manager.add_one(offline_request)
        if AppendResults.APPEND_FAILURE_QUEUE_DISABLED == append_result:
            self._logger.error("Offline request queue has been disabled")
            raise self._offline_request_queue_disabled_exceptions[type]
        if AppendResults.APPEND_FAILURE_QUEUE_FULL == append_result:
            self._logger.error("Offline request queue is full")
            raise self._offline_request_queue_full_exceptions[type]
