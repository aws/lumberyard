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

import importlib
import boto3
import CloudCanvas
import service
import iot_websocket
import iot_openssl
import registration_shared
import channel_list
from datetime import datetime

from registration_shared import client_registry_table_name

@service.api 
def request_channel_list(request):

    print 'Channel list requested from {}'.format(request)
    print 'Client Registry is ' + client_registry_table_name

    try:
        return request_channel_list.player_channel_list
    except:
        player_channel_list = []
        subscription_list = channel_list.get_subscription_channels()

        request_channel_list.player_channel_list = { 'Channels' : subscription_list }
    return request_channel_list.player_channel_list


@service.api     
def request_registration(request, registration_type):
    print 'Registration requested for {} type {}'.format(request, registration_type)

    responseObject = {}

    if registration_type == 'OPENSSL':
        print 'OPENSSL registration not enabled for client request'
    elif registration_type == 'WEBSOCKET':
        responseObject = iot_websocket.register_websocket(request)
    
    responseResult = responseObject.get('Result')
    if responseResult == 'SUCCESS':
        endpoint, endpoint_port = registration_shared.get_endpoint_and_port(registration_type)
    
        responseObject['Endpoint'] = endpoint
        responseObject['EndpointPort'] = endpoint_port
    elif responseResult == None:
        responseObject['Result'] = 'ERROR'
    
    return responseObject
 
