#
#/*
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

from AWSIoTPythonSDK.core.util.providers import CertificateCredentialsProvider
from AWSIoTPythonSDK.core.util.providers import IAMCredentialsProvider
from AWSIoTPythonSDK.core.util.providers import EndpointProvider
from AWSIoTPythonSDK.core.jobs.thingJobManager import jobExecutionTopicType
from AWSIoTPythonSDK.core.jobs.thingJobManager import jobExecutionTopicReplyType
from AWSIoTPythonSDK.core.protocol.mqtt_core import MqttCore
import AWSIoTPythonSDK.core.shadow.shadowManager as shadowManager
import AWSIoTPythonSDK.core.shadow.deviceShadow as deviceShadow
import AWSIoTPythonSDK.core.jobs.thingJobManager as thingJobManager

# Constants
# - Protocol types:
MQTTv3_1 = 3
MQTTv3_1_1 = 4

DROP_OLDEST = 0
DROP_NEWEST = 1

class AWSIoTMQTTClient:

    def __init__(self, clientID, protocolType=MQTTv3_1_1, useWebsocket=False, cleanSession=True):
        """

        The client class that connects to and accesses AWS IoT over MQTT v3.1/3.1.1.

        The following connection types are available:

        - TLSv1.2 Mutual Authentication

        X.509 certificate-based secured MQTT connection to AWS IoT
        
        - Websocket SigV4

        IAM credential-based secured MQTT connection over Websocket to AWS IoT

        It provides basic synchronous MQTT operations in the classic MQTT publish-subscribe 
        model, along with configurations of on-top features:

        - Auto reconnect/resubscribe

        - Progressive reconnect backoff

        - Offline publish requests queueing with draining

        **Syntax**

        .. code:: python

          import AWSIoTPythonSDK.MQTTLib as AWSIoTPyMQTT

          # Create an AWS IoT MQTT Client using TLSv1.2 Mutual Authentication
          myAWSIoTMQTTClient = AWSIoTPyMQTT.AWSIoTMQTTClient("testIoTPySDK")
          # Create an AWS IoT MQTT Client using Websocket SigV4
          myAWSIoTMQTTClient = AWSIoTPyMQTT.AWSIoTMQTTClient("testIoTPySDK", useWebsocket=True)

        **Parameters**

        *clientID* - String that denotes the client identifier used to connect to AWS IoT.
        If empty string were provided, client id for this connection will be randomly generated 
        n server side.

        *protocolType* - MQTT version in use for this connection. Could be :code:`AWSIoTPythonSDK.MQTTLib.MQTTv3_1` or :code:`AWSIoTPythonSDK.MQTTLib.MQTTv3_1_1`

        *useWebsocket* - Boolean that denotes enabling MQTT over Websocket SigV4 or not.

        **Returns**

        :code:`AWSIoTPythonSDK.MQTTLib.AWSIoTMQTTClient` object

        """
        self._mqtt_core = MqttCore(clientID, cleanSession, protocolType, useWebsocket)

    # Configuration APIs
    def configureLastWill(self, topic, payload, QoS, retain=False):
        """
        **Description**

        Used to configure the last will topic, payload and QoS of the client. Should be called before connect.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.configureLastWill("last/Will/Topic", "lastWillPayload", 0)

        **Parameters**

        *topic* - Topic name that last will publishes to.

        *payload* - Payload to publish for last will.

        *QoS* - Quality of Service. Could be 0 or 1.

        **Returns**

        None

        """
        self._mqtt_core.configure_last_will(topic, payload, QoS, retain)

    def clearLastWill(self):
        """
        **Description**

        Used to clear the last will configuration that is previously set through configureLastWill.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.clearLastWill()

        **Parameter**

        None

        **Returns**

        None
        
        """
        self._mqtt_core.clear_last_will()

    def configureEndpoint(self, hostName, portNumber):
        """
        **Description**

        Used to configure the host name and port number the client tries to connect to. Should be called
        before connect.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.configureEndpoint("random.iot.region.amazonaws.com", 8883)

        **Parameters**

        *hostName* - String that denotes the host name of the user-specific AWS IoT endpoint.

        *portNumber* - Integer that denotes the port number to connect to. Could be :code:`8883` for
        TLSv1.2 Mutual Authentication or :code:`443` for Websocket SigV4 and TLSv1.2 Mutual Authentication
        with ALPN extension.

        **Returns**

        None

        """
        endpoint_provider = EndpointProvider()
        endpoint_provider.set_host(hostName)
        endpoint_provider.set_port(portNumber)
        self._mqtt_core.configure_endpoint(endpoint_provider)
        if portNumber == 443 and not self._mqtt_core.use_wss():
            self._mqtt_core.configure_alpn_protocols()

    def configureIAMCredentials(self, AWSAccessKeyID, AWSSecretAccessKey, AWSSessionToken=""):
        """
        **Description**

        Used to configure/update the custom IAM credentials for Websocket SigV4 connection to 
        AWS IoT. Should be called before connect.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.configureIAMCredentials(obtainedAccessKeyID, obtainedSecretAccessKey, obtainedSessionToken)

        .. note::

          Hard-coding credentials into custom script is NOT recommended. Please use AWS Cognito identity service
          or other credential provider.

        **Parameters**

        *AWSAccessKeyID* - AWS Access Key Id from user-specific IAM credentials.

        *AWSSecretAccessKey* - AWS Secret Access Key from user-specific IAM credentials.

        *AWSSessionToken* - AWS Session Token for temporary authentication from STS.

        **Returns**

        None

        """
        iam_credentials_provider = IAMCredentialsProvider()
        iam_credentials_provider.set_access_key_id(AWSAccessKeyID)
        iam_credentials_provider.set_secret_access_key(AWSSecretAccessKey)
        iam_credentials_provider.set_session_token(AWSSessionToken)
        self._mqtt_core.configure_iam_credentials(iam_credentials_provider)

    def configureCredentials(self, CAFilePath, KeyPath="", CertificatePath=""):  # Should be good for MutualAuth certs config and Websocket rootCA config
        """
        **Description**

        Used to configure the rootCA, private key and certificate files. Should be called before connect.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.configureCredentials("PATH/TO/ROOT_CA", "PATH/TO/PRIVATE_KEY", "PATH/TO/CERTIFICATE")

        **Parameters**

        *CAFilePath* - Path to read the root CA file. Required for all connection types.

        *KeyPath* - Path to read the private key. Required for X.509 certificate based connection.

        *CertificatePath* - Path to read the certificate. Required for X.509 certificate based connection.

        **Returns**

        None

        """
        cert_credentials_provider = CertificateCredentialsProvider()
        cert_credentials_provider.set_ca_path(CAFilePath)
        cert_credentials_provider.set_key_path(KeyPath)
        cert_credentials_provider.set_cert_path(CertificatePath)
        self._mqtt_core.configure_cert_credentials(cert_credentials_provider)

    def configureAutoReconnectBackoffTime(self, baseReconnectQuietTimeSecond, maxReconnectQuietTimeSecond, stableConnectionTimeSecond):
        """
        **Description**

        Used to configure the auto-reconnect backoff timing. Should be called before connect.

        **Syntax**

        .. code:: python

          # Configure the auto-reconnect backoff to start with 1 second and use 128 seconds as a maximum back off time.
          # Connection over 20 seconds is considered stable and will reset the back off time back to its base.
          myAWSIoTMQTTClient.configureAutoReconnectBackoffTime(1, 128, 20)

        **Parameters**

        *baseReconnectQuietTimeSecond* - The initial back off time to start with, in seconds. 
        Should be less than the stableConnectionTime.

        *maxReconnectQuietTimeSecond* - The maximum back off time, in seconds.

        *stableConnectionTimeSecond* - The number of seconds for a connection to last to be considered as stable. 
        Back off time will be reset to base once the connection is stable.

        **Returns**

        None

        """
        self._mqtt_core.configure_reconnect_back_off(baseReconnectQuietTimeSecond, maxReconnectQuietTimeSecond, stableConnectionTimeSecond)

    def configureOfflinePublishQueueing(self, queueSize, dropBehavior=DROP_NEWEST):
        """
        **Description**

        Used to configure the queue size and drop behavior for the offline requests queueing. Should be
        called before connect. Queueable offline requests include publish, subscribe and unsubscribe.

        **Syntax**

        .. code:: python

          import AWSIoTPythonSDK.MQTTLib as AWSIoTPyMQTT

          # Configure the offline queue for publish requests to be 20 in size and drop the oldest
           request when the queue is full.
          myAWSIoTMQTTClient.configureOfflinePublishQueueing(20, AWSIoTPyMQTT.DROP_OLDEST)

        **Parameters**

        *queueSize* - Size of the queue for offline publish requests queueing.
         If set to 0, the queue is disabled. If set to -1, the queue size is set to be infinite.

        *dropBehavior* - the type of drop behavior when the queue is full.
         Could be :code:`AWSIoTPythonSDK.core.util.enums.DropBehaviorTypes.DROP_OLDEST` or
         :code:`AWSIoTPythonSDK.core.util.enums.DropBehaviorTypes.DROP_NEWEST`.

        **Returns**

        None

        """
        self._mqtt_core.configure_offline_requests_queue(queueSize, dropBehavior)

    def configureDrainingFrequency(self, frequencyInHz):
        """
        **Description**

        Used to configure the draining speed to clear up the queued requests when the connection is back.
        Should be called before connect.

        **Syntax**

        .. code:: python

          # Configure the draining speed to be 2 requests/second
          myAWSIoTMQTTClient.configureDrainingFrequency(2)

        .. note::

          Make sure the draining speed is fast enough and faster than the publish rate. Slow draining 
          could result in inifinite draining process.

        **Parameters**

        *frequencyInHz* - The draining speed to clear the queued requests, in requests/second.

        **Returns**

        None

        """
        self._mqtt_core.configure_draining_interval_sec(1/float(frequencyInHz))

    def configureConnectDisconnectTimeout(self, timeoutSecond):
        """
        **Description**

        Used to configure the time in seconds to wait for a CONNACK or a disconnect to complete. 
        Should be called before connect.

        **Syntax**

        .. code:: python

          # Configure connect/disconnect timeout to be 10 seconds
          myAWSIoTMQTTClient.configureConnectDisconnectTimeout(10)

        **Parameters**

        *timeoutSecond* - Time in seconds to wait for a CONNACK or a disconnect to complete.

        **Returns**

        None

        """
        self._mqtt_core.configure_connect_disconnect_timeout_sec(timeoutSecond)

    def configureMQTTOperationTimeout(self, timeoutSecond):
        """
        **Description**

        Used to configure the timeout in seconds for MQTT QoS 1 publish, subscribe and unsubscribe. 
        Should be called before connect.

        **Syntax**

        .. code:: python

          # Configure MQTT operation timeout to be 5 seconds
          myAWSIoTMQTTClient.configureMQTTOperationTimeout(5)

        **Parameters**

        *timeoutSecond* - Time in seconds to wait for a PUBACK/SUBACK/UNSUBACK.

        **Returns**

        None

        """
        self._mqtt_core.configure_operation_timeout_sec(timeoutSecond)

    def configureUsernamePassword(self, username, password=None):
        """
        **Description**

        Used to configure the username and password used in CONNECT packet.

        **Syntax**

        .. code:: python

          # Configure user name and password
          myAWSIoTMQTTClient.configureUsernamePassword("myUsername", "myPassword")

        **Parameters**

        *username* - Username used in the username field of CONNECT packet.

        *password* - Password used in the password field of CONNECT packet.

        **Returns**

        None

        """
        self._mqtt_core.configure_username_password(username, password)

    def configureSocketFactory(self, socket_factory):
        """
        **Description**

        Configure a socket factory to custom configure a different socket type for
        mqtt connection. Creating a custom socket allows for configuration of a proxy

        **Syntax**

        .. code:: python

          # Configure socket factory
          custom_args = {"arg1": "val1", "arg2": "val2"}
          socket_factory = lambda: custom.create_connection((host, port), **custom_args)
          myAWSIoTMQTTClient.configureSocketFactory(socket_factory)

        **Parameters**

        *socket_factory* - Anonymous function which creates a custom socket to spec.

        **Returns**

        None

        """
        self._mqtt_core.configure_socket_factory(socket_factory)
        
    def enableMetricsCollection(self):
        """
        **Description**

        Used to enable SDK metrics collection. Username field in CONNECT packet will be used to append the SDK name
        and SDK version in use and communicate to AWS IoT cloud. This metrics collection is enabled by default.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.enableMetricsCollection()

        **Parameters**

        None

        **Returns**

        None

        """
        self._mqtt_core.enable_metrics_collection()

    def disableMetricsCollection(self):
        """
        **Description**

        Used to disable SDK metrics collection.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.disableMetricsCollection()

        **Parameters**

        None

        **Returns**

        None

        """
        self._mqtt_core.disable_metrics_collection()

    # MQTT functionality APIs
    def connect(self, keepAliveIntervalSecond=600):
        """
        **Description**

        Connect to AWS IoT, with user-specific keepalive interval configuration.

        **Syntax**

        .. code:: python

          # Connect to AWS IoT with default keepalive set to 600 seconds
          myAWSIoTMQTTClient.connect()
          # Connect to AWS IoT with keepalive interval set to 1200 seconds
          myAWSIoTMQTTClient.connect(1200)

        **Parameters**

        *keepAliveIntervalSecond* - Time in seconds for interval of sending MQTT ping request.
        A shorter keep-alive interval allows the client to detect disconnects more quickly.
        Default set to 600 seconds.

        **Returns**

        True if the connect attempt succeeded. False if failed.

        """
        self._load_callbacks()
        return self._mqtt_core.connect(keepAliveIntervalSecond)

    def connectAsync(self, keepAliveIntervalSecond=600, ackCallback=None):
        """
        **Description**

        Connect asynchronously to AWS IoT, with user-specific keepalive interval configuration and CONNACK callback.

        **Syntax**

        .. code:: python

          # Connect to AWS IoT with default keepalive set to 600 seconds and a custom CONNACK callback
          myAWSIoTMQTTClient.connectAsync(ackCallback=my_connack_callback)
          # Connect to AWS IoT with default keepalive set to 1200 seconds and a custom CONNACK callback
          myAWSIoTMQTTClient.connectAsync(keepAliveInternvalSecond=1200, ackCallback=myConnackCallback)

        **Parameters**

        *keepAliveIntervalSecond* - Time in seconds for interval of sending MQTT ping request.
        Default set to 600 seconds.

        *ackCallback* - Callback to be invoked when the client receives a CONNACK. Should be in form
        :code:`customCallback(mid, data)`, where :code:`mid` is the packet id for the connect request
        and :code:`data` is the connect result code.

        **Returns**

        Connect request packet id, for tracking purpose in the corresponding callback.

        """
        self._load_callbacks()
        return self._mqtt_core.connect_async(keepAliveIntervalSecond, ackCallback)

    def _load_callbacks(self):
        self._mqtt_core.on_online = self.onOnline
        self._mqtt_core.on_offline = self.onOffline
        self._mqtt_core.on_message = self.onMessage

    def disconnect(self):
        """
        **Description**

        Disconnect from AWS IoT.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.disconnect()

        **Parameters**

        None

        **Returns**

        True if the disconnect attempt succeeded. False if failed.

        """
        return self._mqtt_core.disconnect()

    def disconnectAsync(self, ackCallback=None):
        """
        **Description**

        Disconnect asynchronously to AWS IoT.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.disconnectAsync(ackCallback=myDisconnectCallback)

        **Parameters**

        *ackCallback* - Callback to be invoked when the client finishes sending disconnect and internal clean-up.
        Should be in form :code:`customCallback(mid, data)`, where :code:`mid` is the packet id for the disconnect
        request and :code:`data` is the disconnect result code.

        **Returns**

        Disconnect request packet id, for tracking purpose in the corresponding callback.

        """
        return self._mqtt_core.disconnect_async(ackCallback)

    def publish(self, topic, payload, QoS):
        """
        **Description**

        Publish a new message to the desired topic with QoS.

        **Syntax**

        .. code:: python

          # Publish a QoS0 message "myPayload" to topic "myTopic"
          myAWSIoTMQTTClient.publish("myTopic", "myPayload", 0)
          # Publish a QoS1 message "myPayloadWithQos1" to topic "myTopic/sub"
          myAWSIoTMQTTClient.publish("myTopic/sub", "myPayloadWithQos1", 1)

        **Parameters**

        *topic* - Topic name to publish to.

        *payload* - Payload to publish.

        *QoS* - Quality of Service. Could be 0 or 1.

        **Returns**

        True if the publish request has been sent to paho. False if the request did not reach paho.

        """
        return self._mqtt_core.publish(topic, payload, QoS, False)  # Disable retain for publish by now

    def publishAsync(self, topic, payload, QoS, ackCallback=None):
        """
        **Description**

        Publish a new message asynchronously to the desired topic with QoS and PUBACK callback. Note that the ack
        callback configuration for a QoS0 publish request will be ignored as there are no PUBACK reception.

        **Syntax**

        .. code:: python

          # Publish a QoS0 message "myPayload" to topic "myTopic"
          myAWSIoTMQTTClient.publishAsync("myTopic", "myPayload", 0)
          # Publish a QoS1 message "myPayloadWithQos1" to topic "myTopic/sub", with custom PUBACK callback
          myAWSIoTMQTTClient.publishAsync("myTopic/sub", "myPayloadWithQos1", 1, ackCallback=myPubackCallback)

        **Parameters**

        *topic* - Topic name to publish to.

        *payload* - Payload to publish.

        *QoS* - Quality of Service. Could be 0 or 1.

        *ackCallback* - Callback to be invoked when the client receives a PUBACK. Should be in form
        :code:`customCallback(mid)`, where :code:`mid` is the packet id for the disconnect request.

        **Returns**

        Publish request packet id, for tracking purpose in the corresponding callback.

        """
        return self._mqtt_core.publish_async(topic, payload, QoS, False, ackCallback)

    def subscribe(self, topic, QoS, callback):
        """
        **Description**

        Subscribe to the desired topic and register a callback.

        **Syntax**

        .. code:: python

          # Subscribe to "myTopic" with QoS0 and register a callback
          myAWSIoTMQTTClient.subscribe("myTopic", 0, customCallback)
          # Subscribe to "myTopic/#" with QoS1 and register a callback
          myAWSIoTMQTTClient.subscribe("myTopic/#", 1, customCallback)

        **Parameters**

        *topic* - Topic name or filter to subscribe to.

        *QoS* - Quality of Service. Could be 0 or 1.

        *callback* - Function to be called when a new message for the subscribed topic
        comes in. Should be in form :code:`customCallback(client, userdata, message)`, where
        :code:`message` contains :code:`topic` and :code:`payload`. Note that :code:`client` and :code:`userdata` are
        here just to be aligned with the underneath Paho callback function signature. These fields are pending to be
        deprecated and should not be depended on.

        **Returns**

        True if the subscribe attempt succeeded. False if failed.

        """
        return self._mqtt_core.subscribe(topic, QoS, callback)

    def subscribeAsync(self, topic, QoS, ackCallback=None, messageCallback=None):
        """
        **Description**

        Subscribe to the desired topic and register a message callback with SUBACK callback.

        **Syntax**

        .. code:: python

          # Subscribe to "myTopic" with QoS0, custom SUBACK callback and a message callback
          myAWSIoTMQTTClient.subscribe("myTopic", 0, ackCallback=mySubackCallback, messageCallback=customMessageCallback)
          # Subscribe to "myTopic/#" with QoS1, custom SUBACK callback and a message callback
          myAWSIoTMQTTClient.subscribe("myTopic/#", 1, ackCallback=mySubackCallback, messageCallback=customMessageCallback)

        **Parameters**

        *topic* - Topic name or filter to subscribe to.

        *QoS* - Quality of Service. Could be 0 or 1.

        *ackCallback* - Callback to be invoked when the client receives a SUBACK. Should be in form
        :code:`customCallback(mid, data)`, where :code:`mid` is the packet id for the disconnect request and
        :code:`data` is the granted QoS for this subscription.

        *messageCallback* - Function to be called when a new message for the subscribed topic
        comes in. Should be in form :code:`customCallback(client, userdata, message)`, where
        :code:`message` contains :code:`topic` and :code:`payload`. Note that :code:`client` and :code:`userdata` are
        here just to be aligned with the underneath Paho callback function signature. These fields are pending to be
        deprecated and should not be depended on.

        **Returns**

        Subscribe request packet id, for tracking purpose in the corresponding callback.

        """
        return self._mqtt_core.subscribe_async(topic, QoS, ackCallback, messageCallback)

    def unsubscribe(self, topic):
        """
        **Description**

        Unsubscribe to the desired topic.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.unsubscribe("myTopic")

        **Parameters**

        *topic* - Topic name or filter to unsubscribe to.

        **Returns**

        True if the unsubscribe attempt succeeded. False if failed.

        """
        return self._mqtt_core.unsubscribe(topic)

    def unsubscribeAsync(self, topic, ackCallback=None):
        """
        **Description**

        Unsubscribe to the desired topic with UNSUBACK callback.

        **Syntax**

        .. code:: python

          myAWSIoTMQTTClient.unsubscribe("myTopic", ackCallback=myUnsubackCallback)

        **Parameters**

        *topic* - Topic name or filter to unsubscribe to.

        *ackCallback* - Callback to be invoked when the client receives a UNSUBACK. Should be in form
        :code:`customCallback(mid)`, where :code:`mid` is the packet id for the disconnect request.

        **Returns**

        Unsubscribe request packet id, for tracking purpose in the corresponding callback.

        """
        return self._mqtt_core.unsubscribe_async(topic, ackCallback)

    def onOnline(self):
        """
        **Description**

        Callback that gets called when the client is online. The callback registration should happen before calling
        connect/connectAsync.

        **Syntax**

        .. code:: python

          # Register an onOnline callback
          myAWSIoTMQTTClient.onOnline = myOnOnlineCallback

        **Parameters**

        None

        **Returns**

        None

        """
        pass

    def onOffline(self):
        """
        **Description**

        Callback that gets called when the client is offline. The callback registration should happen before calling
        connect/connectAsync.

        **Syntax**

        .. code:: python

          # Register an onOffline callback
          myAWSIoTMQTTClient.onOffline = myOnOfflineCallback

        **Parameters**

        None

        **Returns**

        None

        """
        pass

    def onMessage(self, message):
        """
        **Description**

        Callback that gets called when the client receives a new message. The callback registration should happen before
        calling connect/connectAsync. This callback, if present, will always be triggered regardless of whether there is
        any message callback registered upon subscribe API call. It is for the purpose to aggregating the processing of
        received messages in one function.

        **Syntax**

        .. code:: python

          # Register an onMessage callback
          myAWSIoTMQTTClient.onMessage = myOnMessageCallback

        **Parameters**

        *message* - Received MQTT message. It contains the source topic as :code:`message.topic`, and the payload as
        :code:`message.payload`.

        **Returns**

        None

        """
        pass

class _AWSIoTMQTTDelegatingClient(object):

    def __init__(self, clientID, protocolType=MQTTv3_1_1, useWebsocket=False, cleanSession=True, awsIoTMQTTClient=None):
        """

        This class is used internally by the SDK and should not be instantiated directly.

        It delegates to a provided AWS IoT MQTT Client or creates a new one given the configuration
        parameters and exposes core operations for subclasses provide convenience methods

        **Syntax**

        None

        **Parameters**

        *clientID* - String that denotes the client identifier used to connect to AWS IoT.
        If empty string were provided, client id for this connection will be randomly generated 
        n server side.

        *protocolType* - MQTT version in use for this connection. Could be :code:`AWSIoTPythonSDK.MQTTLib.MQTTv3_1` or :code:`AWSIoTPythonSDK.MQTTLib.MQTTv3_1_1`

        *useWebsocket* - Boolean that denotes enabling MQTT over Websocket SigV4 or not.

        **Returns**

        AWSIoTPythonSDK.MQTTLib._AWSIoTMQTTDelegatingClient object

        """
        # AWSIOTMQTTClient instance
        self._AWSIoTMQTTClient = awsIoTMQTTClient if awsIoTMQTTClient is not None else AWSIoTMQTTClient(clientID, protocolType, useWebsocket, cleanSession)

    # Configuration APIs
    def configureLastWill(self, topic, payload, QoS):
        """
        **Description**

        Used to configure the last will topic, payload and QoS of the client. Should be called before connect. This is a public
        facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          myShadowClient.configureLastWill("last/Will/Topic", "lastWillPayload", 0)
          myJobsClient.configureLastWill("last/Will/Topic", "lastWillPayload", 0)

        **Parameters**

        *topic* - Topic name that last will publishes to.

        *payload* - Payload to publish for last will.

        *QoS* - Quality of Service. Could be 0 or 1.

        **Returns**

        None

        """
        # AWSIoTMQTTClient.configureLastWill(srcTopic, srcPayload, srcQos)
        self._AWSIoTMQTTClient.configureLastWill(topic, payload, QoS)

    def clearLastWill(self):
        """
        **Description**

        Used to clear the last will configuration that is previously set through configureLastWill. This is a public
        facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          myShadowClient.clearLastWill()
          myJobsClient.clearLastWill()

        **Parameter**

        None

        **Returns**

        None
        
        """
        # AWSIoTMQTTClient.clearLastWill()
        self._AWSIoTMQTTClient.clearLastWill()

    def configureEndpoint(self, hostName, portNumber):
        """
        **Description**

        Used to configure the host name and port number the underneath AWS IoT MQTT Client tries to connect to. Should be called
        before connect. This is a public facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          myShadowClient.clearLastWill("random.iot.region.amazonaws.com", 8883)
          myJobsClient.clearLastWill("random.iot.region.amazonaws.com", 8883)

        **Parameters**

        *hostName* - String that denotes the host name of the user-specific AWS IoT endpoint.

        *portNumber* - Integer that denotes the port number to connect to. Could be :code:`8883` for
        TLSv1.2 Mutual Authentication or :code:`443` for Websocket SigV4 and TLSv1.2 Mutual Authentication
        with ALPN extension.

        **Returns**

        None

        """
        # AWSIoTMQTTClient.configureEndpoint
        self._AWSIoTMQTTClient.configureEndpoint(hostName, portNumber)

    def configureIAMCredentials(self, AWSAccessKeyID, AWSSecretAccessKey, AWSSTSToken=""):
        """
        **Description**

        Used to configure/update the custom IAM credentials for the underneath AWS IoT MQTT Client 
        for Websocket SigV4 connection to AWS IoT. Should be called before connect. This is a public
        facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          myShadowClient.clearLastWill(obtainedAccessKeyID, obtainedSecretAccessKey, obtainedSessionToken)
          myJobsClient.clearLastWill(obtainedAccessKeyID, obtainedSecretAccessKey, obtainedSessionToken)

        .. note::

          Hard-coding credentials into custom script is NOT recommended. Please use AWS Cognito identity service
          or other credential provider.

        **Parameters**

        *AWSAccessKeyID* - AWS Access Key Id from user-specific IAM credentials.

        *AWSSecretAccessKey* - AWS Secret Access Key from user-specific IAM credentials.

        *AWSSessionToken* - AWS Session Token for temporary authentication from STS.

        **Returns**

        None

        """
        # AWSIoTMQTTClient.configureIAMCredentials
        self._AWSIoTMQTTClient.configureIAMCredentials(AWSAccessKeyID, AWSSecretAccessKey, AWSSTSToken)

    def configureCredentials(self, CAFilePath, KeyPath="", CertificatePath=""):  # Should be good for MutualAuth and Websocket
        """
        **Description**

        Used to configure the rootCA, private key and certificate files. Should be called before connect. This is a public
        facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          myShadowClient.clearLastWill("PATH/TO/ROOT_CA", "PATH/TO/PRIVATE_KEY", "PATH/TO/CERTIFICATE")
          myJobsClient.clearLastWill("PATH/TO/ROOT_CA", "PATH/TO/PRIVATE_KEY", "PATH/TO/CERTIFICATE")

        **Parameters**

        *CAFilePath* - Path to read the root CA file. Required for all connection types.

        *KeyPath* - Path to read the private key. Required for X.509 certificate based connection.

        *CertificatePath* - Path to read the certificate. Required for X.509 certificate based connection.

        **Returns**

        None

        """
        # AWSIoTMQTTClient.configureCredentials
        self._AWSIoTMQTTClient.configureCredentials(CAFilePath, KeyPath, CertificatePath)

    def configureAutoReconnectBackoffTime(self, baseReconnectQuietTimeSecond, maxReconnectQuietTimeSecond, stableConnectionTimeSecond):
        """
        **Description**

        Used to configure the auto-reconnect backoff timing. Should be called before connect. This is a public
        facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          # Configure the auto-reconnect backoff to start with 1 second and use 128 seconds as a maximum back off time.
          # Connection over 20 seconds is considered stable and will reset the back off time back to its base.
          myShadowClient.clearLastWill(1, 128, 20)
          myJobsClient.clearLastWill(1, 128, 20)

        **Parameters**

        *baseReconnectQuietTimeSecond* - The initial back off time to start with, in seconds.
        Should be less than the stableConnectionTime.

        *maxReconnectQuietTimeSecond* - The maximum back off time, in seconds.

        *stableConnectionTimeSecond* - The number of seconds for a connection to last to be considered as stable.
        Back off time will be reset to base once the connection is stable.

        **Returns**

        None

        """
        # AWSIoTMQTTClient.configureBackoffTime
        self._AWSIoTMQTTClient.configureAutoReconnectBackoffTime(baseReconnectQuietTimeSecond, maxReconnectQuietTimeSecond, stableConnectionTimeSecond)

    def configureConnectDisconnectTimeout(self, timeoutSecond):
        """
        **Description**

        Used to configure the time in seconds to wait for a CONNACK or a disconnect to complete. 
        Should be called before connect. This is a public facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          # Configure connect/disconnect timeout to be 10 seconds
          myShadowClient.configureConnectDisconnectTimeout(10)
          myJobsClient.configureConnectDisconnectTimeout(10)

        **Parameters**

        *timeoutSecond* - Time in seconds to wait for a CONNACK or a disconnect to complete.

        **Returns**

        None

        """
        # AWSIoTMQTTClient.configureConnectDisconnectTimeout
        self._AWSIoTMQTTClient.configureConnectDisconnectTimeout(timeoutSecond)

    def configureMQTTOperationTimeout(self, timeoutSecond):
        """
        **Description**

        Used to configure the timeout in seconds for MQTT QoS 1 publish, subscribe and unsubscribe. 
        Should be called before connect. This is a public facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          # Configure MQTT operation timeout to be 5 seconds
          myShadowClient.configureMQTTOperationTimeout(5)
          myJobsClient.configureMQTTOperationTimeout(5)

        **Parameters**

        *timeoutSecond* - Time in seconds to wait for a PUBACK/SUBACK/UNSUBACK.

        **Returns**

        None

        """
        # AWSIoTMQTTClient.configureMQTTOperationTimeout
        self._AWSIoTMQTTClient.configureMQTTOperationTimeout(timeoutSecond)

    def configureUsernamePassword(self, username, password=None):
        """
        **Description**

        Used to configure the username and password used in CONNECT packet. This is a public facing API
        inherited by application level public clients.

        **Syntax**

        .. code:: python

          # Configure user name and password
          myShadowClient.configureUsernamePassword("myUsername", "myPassword")
          myJobsClient.configureUsernamePassword("myUsername", "myPassword")

        **Parameters**

        *username* - Username used in the username field of CONNECT packet.

        *password* - Password used in the password field of CONNECT packet.

        **Returns**

        None

        """
        self._AWSIoTMQTTClient.configureUsernamePassword(username, password)

    def configureSocketFactory(self, socket_factory):
        """
        **Description**

        Configure a socket factory to custom configure a different socket type for
        mqtt connection. Creating a custom socket allows for configuration of a proxy

        **Syntax**

        .. code:: python

          # Configure socket factory
          custom_args = {"arg1": "val1", "arg2": "val2"}
          socket_factory = lambda: custom.create_connection((host, port), **custom_args)
          myAWSIoTMQTTClient.configureSocketFactory(socket_factory)

        **Parameters**

        *socket_factory* - Anonymous function which creates a custom socket to spec.

        **Returns**

        None

        """
        self._AWSIoTMQTTClient.configureSocketFactory(socket_factory)
        
    def enableMetricsCollection(self):
        """
        **Description**

        Used to enable SDK metrics collection. Username field in CONNECT packet will be used to append the SDK name
        and SDK version in use and communicate to AWS IoT cloud. This metrics collection is enabled by default.
        This is a public facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          myShadowClient.enableMetricsCollection()
          myJobsClient.enableMetricsCollection()

        **Parameters**

        None

        **Returns**

        None

        """
        self._AWSIoTMQTTClient.enableMetricsCollection()

    def disableMetricsCollection(self):
        """
        **Description**

        Used to disable SDK metrics collection. This is a public facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          myShadowClient.disableMetricsCollection()
          myJobsClient.disableMetricsCollection()

        **Parameters**

        None

        **Returns**

        None

        """
        self._AWSIoTMQTTClient.disableMetricsCollection()

    # Start the MQTT connection
    def connect(self, keepAliveIntervalSecond=600):
        """
        **Description**

        Connect to AWS IoT, with user-specific keepalive interval configuration. This is a public facing API inherited
        by application level public clients.

        **Syntax**

        .. code:: python

          # Connect to AWS IoT with default keepalive set to 600 seconds
          myShadowClient.connect()
          myJobsClient.connect()
          # Connect to AWS IoT with keepalive interval set to 1200 seconds
          myShadowClient.connect(1200)
          myJobsClient.connect(1200)

        **Parameters**

        *keepAliveIntervalSecond* - Time in seconds for interval of sending MQTT ping request. 
        Default set to 30 seconds.

        **Returns**

        True if the connect attempt succeeded. False if failed.

        """
        self._load_callbacks()
        return self._AWSIoTMQTTClient.connect(keepAliveIntervalSecond)

    def _load_callbacks(self):
        self._AWSIoTMQTTClient.onOnline = self.onOnline
        self._AWSIoTMQTTClient.onOffline = self.onOffline

    # End the MQTT connection
    def disconnect(self):
        """
        **Description**

        Disconnect from AWS IoT. This is a public facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          myShadowClient.disconnect()
          myJobsClient.disconnect()

        **Parameters**

        None

        **Returns**

        True if the disconnect attempt succeeded. False if failed.

        """
        return self._AWSIoTMQTTClient.disconnect()

    # MQTT connection management API
    def getMQTTConnection(self):
        """
        **Description**

        Retrieve the AWS IoT MQTT Client used underneath, making it possible to perform
        plain MQTT operations along with specialized operations using the same single connection.
        This is a public facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          # Retrieve the AWS IoT MQTT Client used in the AWS IoT MQTT Delegating Client
          thisAWSIoTMQTTClient = myShadowClient.getMQTTConnection()
          thisAWSIoTMQTTClient = myJobsClient.getMQTTConnection()
          # Perform plain MQTT operations using the same connection
          thisAWSIoTMQTTClient.publish("Topic", "Payload", 1)
          ...

        **Parameters**

        None

        **Returns**

        AWSIoTPythonSDK.MQTTLib.AWSIoTMQTTClient object

        """
        # Return the internal AWSIoTMQTTClient instance
        return self._AWSIoTMQTTClient

    def onOnline(self):
        """
        **Description**

        Callback that gets called when the client is online. The callback registration should happen before calling
        connect. This is a public facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          # Register an onOnline callback
          myShadowClient.onOnline = myOnOnlineCallback
          myJobsClient.onOnline = myOnOnlineCallback

        **Parameters**

        None

        **Returns**

        None

        """
        pass

    def onOffline(self):
        """
        **Description**

        Callback that gets called when the client is offline. The callback registration should happen before calling
        connect. This is a public facing API inherited by application level public clients.

        **Syntax**

        .. code:: python

          # Register an onOffline callback
          myShadowClient.onOffline = myOnOfflineCallback
          myJobsClient.onOffline = myOnOfflineCallback

        **Parameters**

        None

        **Returns**

        None

        """
        pass


class AWSIoTMQTTShadowClient(_AWSIoTMQTTDelegatingClient):

    def __init__(self, clientID, protocolType=MQTTv3_1_1, useWebsocket=False, cleanSession=True, awsIoTMQTTClient=None):
        """

        The client class that manages device shadow and accesses its functionality in AWS IoT over MQTT v3.1/3.1.1.

        It delegates to the AWS IoT MQTT Client and exposes devive shadow related operations.
        It shares the same connection types, synchronous MQTT operations and partial on-top features
        with the AWS IoT MQTT Client:

        - Auto reconnect/resubscribe

        Same as AWS IoT MQTT Client.

        - Progressive reconnect backoff

        Same as AWS IoT MQTT Client.

        - Offline publish requests queueing with draining

        Disabled by default. Queueing is not allowed for time-sensitive shadow requests/messages.

        **Syntax**

        .. code:: python

          import AWSIoTPythonSDK.MQTTLib as AWSIoTPyMQTT

          # Create an AWS IoT MQTT Shadow Client using TLSv1.2 Mutual Authentication
          myAWSIoTMQTTShadowClient = AWSIoTPyMQTT.AWSIoTMQTTShadowClient("testIoTPySDK")
          # Create an AWS IoT MQTT Shadow Client using Websocket SigV4
          myAWSIoTMQTTShadowClient = AWSIoTPyMQTT.AWSIoTMQTTShadowClient("testIoTPySDK",  useWebsocket=True)

        **Parameters**

        *clientID* - String that denotes the client identifier used to connect to AWS IoT.
        If empty string were provided, client id for this connection will be randomly generated
        n server side.

        *protocolType* - MQTT version in use for this connection. Could be :code:`AWSIoTPythonSDK.MQTTLib.MQTTv3_1` or :code:`AWSIoTPythonSDK.MQTTLib.MQTTv3_1_1`

        *useWebsocket* - Boolean that denotes enabling MQTT over Websocket SigV4 or not.

        **Returns**

        AWSIoTPythonSDK.MQTTLib.AWSIoTMQTTShadowClient object

        """
        super(AWSIoTMQTTShadowClient, self).__init__(clientID, protocolType, useWebsocket, cleanSession, awsIoTMQTTClient)
        #leave passed in clients alone
        if awsIoTMQTTClient is None:
            # Configure it to disable offline Publish Queueing
            self._AWSIoTMQTTClient.configureOfflinePublishQueueing(0)  # Disable queueing, no queueing for time-sensitive shadow messages
            self._AWSIoTMQTTClient.configureDrainingFrequency(10)
        # Now retrieve the configured mqttCore and init a shadowManager instance
        self._shadowManager = shadowManager.shadowManager(self._AWSIoTMQTTClient._mqtt_core)

    # Shadow management API
    def createShadowHandlerWithName(self, shadowName, isPersistentSubscribe):
        """
        **Description**

        Create a device shadow handler using the specified shadow name and isPersistentSubscribe.

        **Syntax**

        .. code:: python

          # Create a device shadow handler for shadow named "Bot1", using persistent subscription
          Bot1Shadow = myAWSIoTMQTTShadowClient.createShadowHandlerWithName("Bot1", True)
          # Create a device shadow handler for shadow named "Bot2", using non-persistent subscription
          Bot2Shadow = myAWSIoTMQTTShadowClient.createShadowHandlerWithName("Bot2", False)

        **Parameters**

        *shadowName* - Name of the device shadow.

        *isPersistentSubscribe* - Whether to unsubscribe from shadow response (accepted/rejected) topics
        when there is a response. Will subscribe at the first time the shadow request is made and will
        not unsubscribe if isPersistentSubscribe is set.

        **Returns**

        AWSIoTPythonSDK.core.shadow.deviceShadow.deviceShadow object, which exposes the device shadow interface.

        """
        # Create and return a deviceShadow instance
        return deviceShadow.deviceShadow(shadowName, isPersistentSubscribe, self._shadowManager)
        # Shadow APIs are accessible in deviceShadow instance":
        ###
        # deviceShadow.shadowGet
        # deviceShadow.shadowUpdate
        # deviceShadow.shadowDelete
        # deviceShadow.shadowRegisterDelta
        # deviceShadow.shadowUnregisterDelta

class AWSIoTMQTTThingJobsClient(_AWSIoTMQTTDelegatingClient):

    def __init__(self, clientID, thingName, QoS=0, protocolType=MQTTv3_1_1, useWebsocket=False, cleanSession=True, awsIoTMQTTClient=None):
        """

        The client class that specializes in handling jobs messages and accesses its functionality in AWS IoT over MQTT v3.1/3.1.1.

        It delegates to the AWS IoT MQTT Client and exposes jobs related operations.
        It shares the same connection types, synchronous MQTT operations and partial on-top features
        with the AWS IoT MQTT Client:

        - Auto reconnect/resubscribe

        Same as AWS IoT MQTT Client.

        - Progressive reconnect backoff

        Same as AWS IoT MQTT Client.

        - Offline publish requests queueing with draining

        Same as AWS IoT MQTT Client

        **Syntax**

        .. code:: python

          import AWSIoTPythonSDK.MQTTLib as AWSIoTPyMQTT

          # Create an AWS IoT MQTT Jobs Client using TLSv1.2 Mutual Authentication
          myAWSIoTMQTTJobsClient = AWSIoTPyMQTT.AWSIoTMQTTThingJobsClient("testIoTPySDK")
          # Create an AWS IoT MQTT Jobs Client using Websocket SigV4
          myAWSIoTMQTTJobsClient = AWSIoTPyMQTT.AWSIoTMQTTThingJobsClient("testIoTPySDK",  useWebsocket=True)

        **Parameters**

        *clientID* - String that denotes the client identifier and client token for jobs requests
        If empty string is provided, client id for this connection will be randomly generated
        on server side. If an awsIotMQTTClient is specified, this will not override the client ID
        for the existing MQTT connection and only impact the client token for jobs request payloads

        *thingName* - String that represents the thingName used to send requests to proper topics and subscribe
        to proper topics.

        *QoS* - QoS used for all requests sent through this client

        *awsIoTMQTTClient* - An instance of AWSIoTMQTTClient to use if not None. If not None, clientID, protocolType, useWebSocket,
        and cleanSession parameters are not used. Caller is expected to invoke connect() prior to calling the pub/sub methods on this client.

        *protocolType* - MQTT version in use for this connection. Could be :code:`AWSIoTPythonSDK.MQTTLib.MQTTv3_1` or :code:`AWSIoTPythonSDK.MQTTLib.MQTTv3_1_1`

        *useWebsocket* - Boolean that denotes enabling MQTT over Websocket SigV4 or not.

        **Returns**

        AWSIoTPythonSDK.MQTTLib.AWSIoTMQTTJobsClient object

        """
        # AWSIOTMQTTClient instance
        super(AWSIoTMQTTThingJobsClient, self).__init__(clientID, protocolType, useWebsocket, cleanSession, awsIoTMQTTClient)
        self._thingJobManager = thingJobManager.thingJobManager(thingName, clientID)
        self._QoS = QoS

    def createJobSubscription(self, callback, jobExecutionType=jobExecutionTopicType.JOB_WILDCARD_TOPIC, jobReplyType=jobExecutionTopicReplyType.JOB_REQUEST_TYPE, jobId=None):
        """
        **Description**

        Synchronously creates an MQTT subscription to a jobs related topic based on the provided arguments

        **Syntax**

        .. code:: python

          #Subscribe to notify-next topic to monitor change in job referred to by $next
          myAWSIoTMQTTJobsClient.createJobSubscription(callback, jobExecutionTopicType.JOB_NOTIFY_NEXT_TOPIC)
          #Subscribe to notify topic to monitor changes to jobs in pending list
          myAWSIoTMQTTJobsClient.createJobSubscription(callback, jobExecutionTopicType.JOB_NOTIFY_TOPIC)
          #Subscribe to receive messages for job execution updates
          myAWSIoTMQTTJobsClient.createJobSubscription(callback, jobExecutionTopicType.JOB_UPDATE_TOPIC, jobExecutionTopicReplyType.JOB_ACCEPTED_REPLY_TYPE)
          #Subscribe to receive messages for describing a job execution
          myAWSIoTMQTTJobsClient.createJobSubscription(callback, jobExecutionTopicType.JOB_DESCRIBE_TOPIC, jobExecutionTopicReplyType.JOB_ACCEPTED_REPLY_TYPE, jobId)

        **Parameters**

        *callback* - Function to be called when a new message for the subscribed job topic
        comes in. Should be in form :code:`customCallback(client, userdata, message)`, where
        :code:`message` contains :code:`topic` and :code:`payload`. Note that :code:`client` and :code:`userdata` are
        here just to be aligned with the underneath Paho callback function signature. These fields are pending to be
        deprecated and should not be depended on.

        *jobExecutionType* - Member of the jobExecutionTopicType class specifying the jobs topic to subscribe to
        Defaults to jobExecutionTopicType.JOB_WILDCARD_TOPIC

        *jobReplyType* - Member of the jobExecutionTopicReplyType class specifying the (optional) reply sub-topic to subscribe to
        Defaults to jobExecutionTopicReplyType.JOB_REQUEST_TYPE which indicates the subscription isn't intended for a jobs reply topic

        *jobId* - JobId string  if the topic type requires one.
        Defaults to None

        **Returns**

        True if the subscribe attempt succeeded. False if failed.

        """
        topic = self._thingJobManager.getJobTopic(jobExecutionType, jobReplyType, jobId)
        return self._AWSIoTMQTTClient.subscribe(topic, self._QoS, callback)

    def createJobSubscriptionAsync(self, ackCallback, callback, jobExecutionType=jobExecutionTopicType.JOB_WILDCARD_TOPIC, jobReplyType=jobExecutionTopicReplyType.JOB_REQUEST_TYPE, jobId=None):
        """
        **Description**

        Asynchronously creates an MQTT subscription to a jobs related topic based on the provided arguments

        **Syntax**

        .. code:: python

          #Subscribe to notify-next topic to monitor change in job referred to by $next
          myAWSIoTMQTTJobsClient.createJobSubscriptionAsync(callback, jobExecutionTopicType.JOB_NOTIFY_NEXT_TOPIC)
          #Subscribe to notify topic to monitor changes to jobs in pending list
          myAWSIoTMQTTJobsClient.createJobSubscriptionAsync(callback, jobExecutionTopicType.JOB_NOTIFY_TOPIC)
          #Subscribe to receive messages for job execution updates
          myAWSIoTMQTTJobsClient.createJobSubscriptionAsync(callback, jobExecutionTopicType.JOB_UPDATE_TOPIC, jobExecutionTopicReplyType.JOB_ACCEPTED_REPLY_TYPE)
          #Subscribe to receive messages for describing a job execution
          myAWSIoTMQTTJobsClient.createJobSubscriptionAsync(callback, jobExecutionTopicType.JOB_DESCRIBE_TOPIC, jobExecutionTopicReplyType.JOB_ACCEPTED_REPLY_TYPE, jobId)

        **Parameters**

        *ackCallback* - Callback to be invoked when the client receives a SUBACK. Should be in form
        :code:`customCallback(mid, data)`, where :code:`mid` is the packet id for the disconnect request and
        :code:`data` is the granted QoS for this subscription.

        *callback* - Function to be called when a new message for the subscribed job topic
        comes in. Should be in form :code:`customCallback(client, userdata, message)`, where
        :code:`message` contains :code:`topic` and :code:`payload`. Note that :code:`client` and :code:`userdata` are
        here just to be aligned with the underneath Paho callback function signature. These fields are pending to be
        deprecated and should not be depended on.

        *jobExecutionType* - Member of the jobExecutionTopicType class specifying the jobs topic to subscribe to
        Defaults to jobExecutionTopicType.JOB_WILDCARD_TOPIC

        *jobReplyType* - Member of the jobExecutionTopicReplyType class specifying the (optional) reply sub-topic to subscribe to
        Defaults to jobExecutionTopicReplyType.JOB_REQUEST_TYPE which indicates the subscription isn't intended for a jobs reply topic

        *jobId* - JobId of the topic if the topic type requires one.
        Defaults to None

        **Returns**

        Subscribe request packet id, for tracking purpose in the corresponding callback.

        """
        topic = self._thingJobManager.getJobTopic(jobExecutionType, jobReplyType, jobId)
        return self._AWSIoTMQTTClient.subscribeAsync(topic, self._QoS, ackCallback, callback)

    def sendJobsQuery(self, jobExecTopicType, jobId=None):
        """
        **Description**

        Publishes an MQTT jobs related request for a potentially specific jobId (or wildcard)

        **Syntax**

        .. code:: python

          #send a request to describe the next job
          myAWSIoTMQTTJobsClient.sendJobsQuery(jobExecutionTopicType.JOB_DESCRIBE_TOPIC, '$next')
          #send a request to get list of pending jobs
          myAWSIoTMQTTJobsClient.sendJobsQuery(jobExecutionTopicType.JOB_GET_PENDING_TOPIC)

        **Parameters**

        *jobExecutionType* - Member of the jobExecutionTopicType class that correlates the jobs topic to publish to

        *jobId* - JobId string if the topic type requires one.
        Defaults to None

        **Returns**

        True if the publish request has been sent to paho. False if the request did not reach paho.

        """
        topic = self._thingJobManager.getJobTopic(jobExecTopicType, jobExecutionTopicReplyType.JOB_REQUEST_TYPE, jobId)
        payload = self._thingJobManager.serializeClientTokenPayload()
        return self._AWSIoTMQTTClient.publish(topic, payload, self._QoS)

    def sendJobsStartNext(self, statusDetails=None, stepTimeoutInMinutes=None):
        """
        **Description**

        Publishes an MQTT message to the StartNextJobExecution topic. This will attempt to get the next pending
        job execution and change its status to IN_PROGRESS.

        **Syntax**

        .. code:: python

          #Start next job (set status to IN_PROGRESS) and update with optional statusDetails
          myAWSIoTMQTTJobsClient.sendJobsStartNext({'StartedBy': 'myClientId'})

        **Parameters**

        *statusDetails* - Dictionary containing the key value pairs to use for the status details of the job execution

        *stepTimeoutInMinutes - Specifies the amount of time this device has to finish execution of this job. 

        **Returns**

        True if the publish request has been sent to paho. False if the request did not reach paho.

        """
        topic = self._thingJobManager.getJobTopic(jobExecutionTopicType.JOB_START_NEXT_TOPIC, jobExecutionTopicReplyType.JOB_REQUEST_TYPE)
        payload = self._thingJobManager.serializeStartNextPendingJobExecutionPayload(statusDetails, stepTimeoutInMinutes)
        return self._AWSIoTMQTTClient.publish(topic, payload, self._QoS)

    def sendJobsUpdate(self, jobId, status, statusDetails=None, expectedVersion=0, executionNumber=0, includeJobExecutionState=False, includeJobDocument=False, stepTimeoutInMinutes=None):
        """
        **Description**

        Publishes an MQTT message to a corresponding job execution specific topic to update its status according to the parameters.
        Can be used to change a job from QUEUED to IN_PROGRESS to SUCEEDED or FAILED.

        **Syntax**

        .. code:: python

          #Update job with id 'jobId123' to succeeded state, specifying new status details, with expectedVersion=1, executionNumber=2.
          #For the response, include job execution state and not the job document
          myAWSIoTMQTTJobsClient.sendJobsUpdate('jobId123', jobExecutionStatus.JOB_EXECUTION_SUCCEEDED, statusDetailsMap, 1, 2, True, False)


          #Update job with id 'jobId456' to failed state
          myAWSIoTMQTTJobsClient.sendJobsUpdate('jobId456', jobExecutionStatus.JOB_EXECUTION_FAILED)

        **Parameters**

        *jobId* - JobID String of the execution to update the status of

        *status* - job execution status to change the job execution to. Member of jobExecutionStatus

        *statusDetails* - new status details to set on the job execution

        *expectedVersion* - The expected current version of the job execution. IoT jobs increments expectedVersion each time you update the job execution.
        If the version of the job execution stored in Jobs does not match, the update is rejected with a VersionMismatch error, and an ErrorResponse
        that contains the current job execution status data is returned. (This makes it unnecessary to perform a separate DescribeJobExecution request
        n order to obtain the job execution status data.)

        *executionNumber* - A number that identifies a particular job execution on a particular device. If not specified, the latest job execution is used.

        *includeJobExecutionState* - When included and set to True, the response contains the JobExecutionState field. The default is False.

        *includeJobDocument* - When included and set to True, the response contains the JobDocument. The default is False.

        *stepTimeoutInMinutes - Specifies the amount of time this device has to finish execution of this job. 

        **Returns**

        True if the publish request has been sent to paho. False if the request did not reach paho.

        """
        topic = self._thingJobManager.getJobTopic(jobExecutionTopicType.JOB_UPDATE_TOPIC, jobExecutionTopicReplyType.JOB_REQUEST_TYPE, jobId)
        payload = self._thingJobManager.serializeJobExecutionUpdatePayload(status, statusDetails, expectedVersion, executionNumber, includeJobExecutionState, includeJobDocument, stepTimeoutInMinutes)
        return self._AWSIoTMQTTClient.publish(topic, payload, self._QoS)

    def sendJobsDescribe(self, jobId, executionNumber=0, includeJobDocument=True):
        """
        **Description**

        Publishes a method to the describe topic for a particular job.

        **Syntax**

        .. code:: python

          #Describe job with id 'jobId1' of any executionNumber, job document will be included in response
          myAWSIoTMQTTJobsClient.sendJobsDescribe('jobId1')

          #Describe job with id 'jobId2', with execution number of 2, and includeJobDocument in the response
          myAWSIoTMQTTJobsClient.sendJobsDescribe('jobId2', 2, True)

        **Parameters**

        *jobId* - jobID to describe. This is allowed to be a wildcard such as '$next'

        *executionNumber* - A number that identifies a particular job execution on a particular device. If not specified, the latest job execution is used.

        *includeJobDocument* - When included and set to True, the response contains the JobDocument.

        **Returns**

        True if the publish request has been sent to paho. False if the request did not reach paho.

        """
        topic = self._thingJobManager.getJobTopic(jobExecutionTopicType.JOB_DESCRIBE_TOPIC, jobExecutionTopicReplyType.JOB_REQUEST_TYPE, jobId)
        payload = self._thingJobManager.serializeDescribeJobExecutionPayload(executionNumber, includeJobDocument)
        return self._AWSIoTMQTTClient.publish(topic, payload, self._QoS)
