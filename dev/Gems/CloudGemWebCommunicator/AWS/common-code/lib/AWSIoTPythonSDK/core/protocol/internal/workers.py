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

import time
import logging
from threading import Thread
from threading import Event
from AWSIoTPythonSDK.core.protocol.internal.events import EventTypes
from AWSIoTPythonSDK.core.protocol.internal.events import FixedEventMids
from AWSIoTPythonSDK.core.protocol.internal.clients import ClientStatus
from AWSIoTPythonSDK.core.protocol.internal.queues import OfflineRequestQueue
from AWSIoTPythonSDK.core.protocol.internal.requests import RequestTypes
from AWSIoTPythonSDK.core.protocol.paho.client import topic_matches_sub
from AWSIoTPythonSDK.core.protocol.internal.defaults import DEFAULT_DRAINING_INTERNAL_SEC


class EventProducer(object):

    _logger = logging.getLogger(__name__)

    def __init__(self, cv, event_queue):
        self._cv = cv
        self._event_queue = event_queue

    def on_connect(self, client, user_data, flags, rc):
        self._add_to_queue(FixedEventMids.CONNACK_MID, EventTypes.CONNACK, rc)
        self._logger.debug("Produced [connack] event")

    def on_disconnect(self, client, user_data, rc):
        self._add_to_queue(FixedEventMids.DISCONNECT_MID, EventTypes.DISCONNECT, rc)
        self._logger.debug("Produced [disconnect] event")

    def on_publish(self, client, user_data, mid):
        self._add_to_queue(mid, EventTypes.PUBACK, None)
        self._logger.debug("Produced [puback] event")

    def on_subscribe(self, client, user_data, mid, granted_qos):
        self._add_to_queue(mid, EventTypes.SUBACK, granted_qos)
        self._logger.debug("Produced [suback] event")

    def on_unsubscribe(self, client, user_data, mid):
        self._add_to_queue(mid, EventTypes.UNSUBACK, None)
        self._logger.debug("Produced [unsuback] event")

    def on_message(self, client, user_data, message):
        self._add_to_queue(FixedEventMids.MESSAGE_MID, EventTypes.MESSAGE, message)
        self._logger.debug("Produced [message] event")

    def _add_to_queue(self, mid, event_type, data):
        with self._cv:
            self._event_queue.put((mid, event_type, data))
            self._cv.notify()


class EventConsumer(object):

    MAX_DISPATCH_INTERNAL_SEC = 0.01
    _logger = logging.getLogger(__name__)

    def __init__(self, cv, event_queue, internal_async_client,
                 subscription_manager, offline_requests_manager, client_status):
        self._cv = cv
        self._event_queue = event_queue
        self._internal_async_client = internal_async_client
        self._subscription_manager = subscription_manager
        self._offline_requests_manager = offline_requests_manager
        self._client_status = client_status
        self._is_running = False
        self._draining_interval_sec = DEFAULT_DRAINING_INTERNAL_SEC
        self._dispatch_methods = {
            EventTypes.CONNACK : self._dispatch_connack,
            EventTypes.DISCONNECT : self._dispatch_disconnect,
            EventTypes.PUBACK : self._dispatch_puback,
            EventTypes.SUBACK : self._dispatch_suback,
            EventTypes.UNSUBACK : self._dispatch_unsuback,
            EventTypes.MESSAGE : self._dispatch_message
        }
        self._offline_request_handlers = {
            RequestTypes.PUBLISH : self._handle_offline_publish,
            RequestTypes.SUBSCRIBE : self._handle_offline_subscribe,
            RequestTypes.UNSUBSCRIBE : self._handle_offline_unsubscribe
        }
        self._stopper = Event()

    def update_offline_requests_manager(self, offline_requests_manager):
        self._offline_requests_manager = offline_requests_manager

    def update_draining_interval_sec(self, draining_interval_sec):
        self._draining_interval_sec = draining_interval_sec

    def get_draining_interval_sec(self):
        return self._draining_interval_sec

    def is_running(self):
        return self._is_running

    def start(self):
        self._stopper.clear()
        self._is_running = True
        dispatch_events = Thread(target=self._dispatch)
        dispatch_events.daemon = True
        dispatch_events.start()
        self._logger.debug("Event consuming thread started")

    def stop(self):
        if self._is_running:
            self._is_running = False
            self._clean_up()
        self._logger.debug("Event consuming thread stopped")

    def _clean_up(self):
        self._logger.debug("Cleaning up before stopping event consuming")
        with self._event_queue.mutex:
            self._event_queue.queue.clear()
            self._logger.debug("Event queue cleared")
        self._internal_async_client.stop_background_network_io()
        self._logger.debug("Network thread stopped")
        self._internal_async_client.clean_up_event_callbacks()
        self._logger.debug("Event callbacks cleared")

    def wait_until_it_stops(self, timeout_sec):
        self._logger.debug("Waiting for event consumer to completely stop")
        return self._stopper.wait(timeout=timeout_sec)

    def is_fully_stopped(self):
        return self._stopper.is_set()

    def _dispatch(self):
        while self._is_running:
            with self._cv:
                if self._event_queue.empty():
                    self._cv.wait(self.MAX_DISPATCH_INTERNAL_SEC)
                else:
                    while not self._event_queue.empty():
                        self._dispatch_one()
        self._stopper.set()
        self._logger.debug("Exiting dispatching loop...")

    def _dispatch_one(self):
        mid, event_type, data = self._event_queue.get()
        if mid:
            self._dispatch_methods[event_type](mid, data)
            self._internal_async_client.invoke_event_callback(mid, data=data)
            # We need to make sure disconnect event gets dispatched and then we stop the consumer
            if self._need_to_stop_dispatching(mid):
                self.stop()

    def _need_to_stop_dispatching(self, mid):
        status = self._client_status.get_status()
        return (ClientStatus.USER_DISCONNECT == status or ClientStatus.CONNECT == status) \
               and mid == FixedEventMids.DISCONNECT_MID

    def _dispatch_connack(self, mid, rc):
        status = self._client_status.get_status()
        self._logger.debug("Dispatching [connack] event")
        if self._need_recover():
            if ClientStatus.STABLE != status:  # To avoid multiple connack dispatching
                self._logger.debug("Has recovery job")
                clean_up_debt = Thread(target=self._clean_up_debt)
                clean_up_debt.start()
        else:
            self._logger.debug("No need for recovery")
            self._client_status.set_status(ClientStatus.STABLE)

    def _need_recover(self):
        return self._subscription_manager.list_records() or self._offline_requests_manager.has_more()

    def _clean_up_debt(self):
        self._handle_resubscribe()
        self._handle_draining()
        self._client_status.set_status(ClientStatus.STABLE)

    def _handle_resubscribe(self):
        subscriptions = self._subscription_manager.list_records()
        if subscriptions and not self._has_user_disconnect_request():
            self._logger.debug("Start resubscribing")
            self._client_status.set_status(ClientStatus.RESUBSCRIBE)
            for topic, (qos, message_callback, ack_callback) in subscriptions:
                if self._has_user_disconnect_request():
                    self._logger.debug("User disconnect detected")
                    break
                self._internal_async_client.subscribe(topic, qos, ack_callback)

    def _handle_draining(self):
        if self._offline_requests_manager.has_more() and not self._has_user_disconnect_request():
            self._logger.debug("Start draining")
            self._client_status.set_status(ClientStatus.DRAINING)
            while self._offline_requests_manager.has_more():
                if self._has_user_disconnect_request():
                    self._logger.debug("User disconnect detected")
                    break
                offline_request = self._offline_requests_manager.get_next()
                if offline_request:
                    self._offline_request_handlers[offline_request.type](offline_request)
                    time.sleep(self._draining_interval_sec)

    def _has_user_disconnect_request(self):
        return ClientStatus.USER_DISCONNECT == self._client_status.get_status()

    def _dispatch_disconnect(self, mid, rc):
        self._logger.debug("Dispatching [disconnect] event")
        status = self._client_status.get_status()
        if ClientStatus.USER_DISCONNECT == status or ClientStatus.CONNECT == status:
            pass
        else:
            self._client_status.set_status(ClientStatus.ABNORMAL_DISCONNECT)

    # For puback, suback and unsuback, ack callback invocation is handled in dispatch_one
    # Do nothing in the event dispatching itself
    def _dispatch_puback(self, mid, rc):
        self._logger.debug("Dispatching [puback] event")

    def _dispatch_suback(self, mid, rc):
        self._logger.debug("Dispatching [suback] event")

    def _dispatch_unsuback(self, mid, rc):
        self._logger.debug("Dispatching [unsuback] event")

    def _dispatch_message(self, mid, message):
        self._logger.debug("Dispatching [message] event")
        subscriptions = self._subscription_manager.list_records()
        if subscriptions:
            for topic, (qos, message_callback, _) in subscriptions:
                if topic_matches_sub(topic, message.topic) and message_callback:
                    message_callback(None, None, message)  # message_callback(client, userdata, message)

    def _handle_offline_publish(self, request):
        topic, payload, qos, retain = request.data
        self._internal_async_client.publish(topic, payload, qos, retain)
        self._logger.debug("Processed offline publish request")

    def _handle_offline_subscribe(self, request):
        topic, qos, message_callback, ack_callback = request.data
        self._subscription_manager.add_record(topic, qos, message_callback, ack_callback)
        self._internal_async_client.subscribe(topic, qos, ack_callback)
        self._logger.debug("Processed offline subscribe request")

    def _handle_offline_unsubscribe(self, request):
        topic, ack_callback = request.data
        self._subscription_manager.remove_record(topic)
        self._internal_async_client.unsubscribe(topic, ack_callback)
        self._logger.debug("Processed offline unsubscribe request")


class SubscriptionManager(object):

    _logger = logging.getLogger(__name__)

    def __init__(self):
        self._subscription_map = dict()

    def add_record(self, topic, qos, message_callback, ack_callback):
        self._logger.debug("Adding a new subscription record: %s qos: %d", topic, qos)
        self._subscription_map[topic] = qos, message_callback, ack_callback  # message_callback and/or ack_callback could be None

    def remove_record(self, topic):
        self._logger.debug("Removing subscription record: %s", topic)
        if self._subscription_map.get(topic):  # Ignore topics that are never subscribed to
            del self._subscription_map[topic]
        else:
            self._logger.warn("Removing attempt for non-exist subscription record: %s", topic)

    def list_records(self):
        return list(self._subscription_map.items())


class OfflineRequestsManager(object):

    _logger = logging.getLogger(__name__)

    def __init__(self, max_size, drop_behavior):
        self._queue = OfflineRequestQueue(max_size, drop_behavior)

    def has_more(self):
        return len(self._queue) > 0

    def add_one(self, request):
        return self._queue.append(request)

    def get_next(self):
        if self.has_more():
            return self._queue.pop(0)
        else:
            return None
