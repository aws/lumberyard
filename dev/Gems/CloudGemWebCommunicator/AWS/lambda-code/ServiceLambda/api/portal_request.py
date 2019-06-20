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
import errors
import messaging
import iot_websocket
import registration_shared
import channel_list
import iot_request
import iot_openssl
from botocore.exceptions import ClientError
from registration_shared import listener_policy, cgp_listener_policy, device_policy
from registration_shared import client_registry_table
from registration_shared import iot_client
from registration_shared import iot_data

@service.api 
def channel_broadcast(request, message_info):

    print 'Channel broadcast requested from {}'.format(request)
    print 'Message info is {}'.format(message_info)
    
    channel_name = message_info.get('ChannelName')
    message_text = message_info.get('Message')

    messaging.broadcast_message(channel_name, message_text)
    
    return { 'Result': 'Message Sent' }


@service.api
def list_all_users(request):
    table_data = client_registry_table.scan()
    resultData = []

    this_result = {}
    for item_data in table_data['Items']:
        print 'Found item {}'.format(item_data)
        resultData.append(item_data)

    while table_data.get('LastEvaluatedKey'):
        table_data = client_registry_table.scan(ExclusiveStartKey=table_data.get('LastEvaluatedKey'))

        this_result = {}
        for item_data in table_data['Items']:
            print 'Found item {}'.format(item_data)
            resultData.append(item_data)

    return {'UserInfoList': resultData}


def _register_user(clientId):
    print 'Registering user {}'.format(clientId)
    client_info = registration_shared.get_user_entry(clientId)

    if not client_info:
        print 'Attempting to register invalid user {}'.format(clientId)
        return

    if client_info.get('CGPClient'):
        policy_name = cgp_listener_policy
    elif client_info.get('CertificateARN'):
        policy_name = device_policy
    else:
        policy_name = listener_policy

    #Cognito Users' policies are attached to CognitoID, Certificate Permissions are attached to CertificateARN
    principalId = client_info.get('CertificateARN',clientId)
    iot_client.attach_policy(target=principalId, policyName=policy_name)

def _ban_user(clientId):
    print 'Banning user {}'.format(clientId)
    client_info = registration_shared.get_user_entry(clientId)

    if not client_info:
        print 'Attempting to ban invalid user {}'.format(clientId)
        return

    if client_info.get('CGPClient'):
        policy_name = cgp_listener_policy
    elif client_info.get('CertificateARN'):
        policy_name = device_policy
    else:
        policy_name = listener_policy

    #Cognito Users' policies are attached to CognitoID, Certificate Permissions are attached to CertificateARN
    principalId = client_info.get('CertificateARN',clientId)
    iot_client.detach_policy(target=principalId, policyName=policy_name)

    iot_request.force_disconnect(clientId)

@service.api
def set_user_status(request, request_content):

    print 'Got set user status request {}'.format(request_content)

    userID = request_content.get('ClientID')
    if userID is None:
        raise errors.ClientError('No ClientID specified')

    attribute_updates = ''
    expression_attribute_values = {}
    new_status = request_content.get('RegistrationStatus')
    if new_status is None:
        raise errors.ClientError('No RegistrationStatus specified')

    if not new_status in ['BANNED', 'REGISTERED']:
        raise errors.ClientError('Invalid status: {}'.format(new_status))

    if new_status == 'BANNED':
        _ban_user(userID)
    elif new_status == 'REGISTERED':
        _register_user(userID)
    else:
        raise errors.ClientError('Invalid status: {}'.format(new_status))

    attribute_updates += ' RegistrationStatus = :status'
    expression_attribute_values[':status'] = new_status

    update_expression = '{} {}'.format('SET', attribute_updates)
    try:
        response_info = client_registry_table.update_item(Key={'ClientID': userID}, UpdateExpression=update_expression,
                                                  ExpressionAttributeValues=expression_attribute_values,
                                                  ReturnValues='ALL_NEW')
    except ClientError as e:
        raise errors.ClientError(e.response['Error']['Message'])

    returned_item = response_info.get('Attributes', {})

    return {'SetUserStatusResult': returned_item}

@service.api     
def request_cgp_registration(request, registration_type):
    print 'Portal Registration requested for {} type {}'.format(request, registration_type)

    responseObject = {}

    if registration_type == 'OPENSSL':
        responseObject = iot_openssl.register_openssl(request)
    elif registration_type == 'WEBSOCKET':
        responseObject = iot_websocket.register_websocket(request, True)
    else:
        print 'Invalid Registration type {}'.format(registration_type)

    responseResult = responseObject.get('Result')
    if responseResult == 'SUCCESS':
        endpoint, endpoint_port = registration_shared.get_endpoint_and_port(registration_type)
    
        responseObject['Endpoint'] = endpoint
        responseObject['EndpointPort'] = endpoint_port
    elif responseResult == None:
        responseObject['Result'] = 'ERROR'
    
    return responseObject


@service.api 
def list_all_channels(request):
            
    return channel_list.get_full_channel_list() 