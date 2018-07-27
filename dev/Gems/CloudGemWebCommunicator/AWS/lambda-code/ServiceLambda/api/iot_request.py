#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
import registration_shared
import os

def force_disconnect(client_id):
    print 'Attempting to force disconnect on client {}'.format(client_id)
    endpoint, endpoint_port = registration_shared.get_endpoint_and_port('WEBSOCKET')

    print 'Using endpoint {}:{}'.format(endpoint, endpoint_port)

    iot_client = AWSIoTMQTTClient(client_id, useWebsocket=True)
    iot_client.configureEndpoint(endpoint, endpoint_port)
    iot_client.configureConnectDisconnectTimeout(20)
    iot_client.configureMQTTOperationTimeout(5)
    iot_client.configureCredentials('AWSIoTPythonSDK/certs/rootCA.pem')

    access_key = os.environ.get('AWS_ACCESS_KEY_ID')
    secret_key = os.environ.get('AWS_SECRET_ACCESS_KEY')
    session_token = os.environ.get('AWS_SESSION_TOKEN')
    iot_client.configureIAMCredentials(AWSAccessKeyID=access_key, AWSSecretAccessKey=secret_key, AWSSessionToken=session_token)

    print 'Configured credentials, attempting connect...'
    response = iot_client.connect()

    print 'Connect returns {}'.format(response)

    if response:
        print 'Disconnecting..'
        iot_client.disconnect()