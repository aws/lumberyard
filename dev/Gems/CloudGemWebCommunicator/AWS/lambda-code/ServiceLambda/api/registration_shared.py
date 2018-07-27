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
from datetime import datetime

client_registry_table_name = CloudCanvas.get_setting('ClientRegistry')
listener_policy = CloudCanvas.get_setting('IotPlayerPolicy')
cgp_listener_policy = CloudCanvas.get_setting('IotCgpPolicy')   
device_policy = CloudCanvas.get_setting('IotDevicePolicy')

client_registry_table = boto3.resource('dynamodb').Table(client_registry_table_name)
 
iot_client = boto3.client('iot')
iot_data = boto3.client('iot-data')

default_connection_type = 'OPENSSL'

def get_endpoint_and_port(connection_type):
    endpoint_response = iot_client.describe_endpoint()
    
    if connection_type == 'OPENSSL':
        endpoint_port = 8883
    elif connection_type == 'WEBSOCKET':
        endpoint_port = 443
        
    return endpoint_response.get('endpointAddress'), endpoint_port


def get_user_entry(cognitoId):
    resultData = {}

    response_data = client_registry_table.get_item(
        Key={
            'ClientID': cognitoId
        })

    if response_data is None:
        resultData['RegistrationStatus'] = 'NEW_USER'
        return resultData
        
    resultData = response_data.get('Item', {})

    return resultData


def _get_time_format():
    return '%b %d %Y %H:%M'


def get_formatted_time_string(timeval):
    return datetime.strftime(timeval, _get_time_format())


def show_principal_policies(cognitoId):
    response = iot_client.list_principal_policies(principal=cognitoId)
    
    policyList = response['policies']
    print 'Policies on principal {} : {}'.format(cognitoId, policyList)
    
    for thisPolicy in policyList:
        print 'Policy: {}'.format(thisPolicy.get('policyName'))


def check_add_policy(cognitoId, policyname):
    print 'Attempting to add policy {}'.format(policyname)
    response = iot_client.list_principal_policies(principal=cognitoId)
    
    policyList = response['policies']
    
    for thisPolicy in policyList:
        if(thisPolicy.get('policyName') == policyname):
            print 'Policy {} Already on principal {}, removing and re-adding'.format(policyname, cognitoId)
            iot_client.detach_principal_policy(policyName=policyname,principal=cognitoId)
            response = iot_client.attach_principal_policy(policyName=policyname,principal=cognitoId)
            print 'Readding policy returns {}'.format(response)
            return
    
    response = iot_client.attach_principal_policy(policyName=policyname,principal=cognitoId)
    
    print 'Policy {} not found on principal {} - Adding returned {}'.format(policyname, cognitoId,response)
        
        
def clear_principal_policies(cognitoId):
    response = iot_client.list_principal_policies(principal=cognitoId)
    
    policyList = response['policies']
    print 'Policies on principal {} : {}'.format(cognitoId, policyList)
    
    for thisPolicy in policyList:
        iot_client.detach_principal_policy(policyName=thisPolicy.get('policyName'),principal=cognitoId)
        print 'Detached {} from {}'.format(thisPolicy.get('policyName'), cognitoId)


def create_user_entry(clientId, status, cgp = False, certificateArn = None):

    print 'Creating User entry for {} - status {}'.format(clientId, status)
    
    attribute_updates = ''
    expression_attribute_values = {}
    new_status = status
    if new_status is not None:
        attribute_updates += ' RegistrationStatus = :status'
        expression_attribute_values[':status'] = new_status         
            
    time_start = get_formatted_time_string(datetime.utcnow())
    if len(attribute_updates):
        attribute_updates += ', '
    attribute_updates += ' RegistrationDate = :datetime'
    expression_attribute_values[':datetime'] = time_start

    if len(attribute_updates):
        attribute_updates += ', '
    attribute_updates += ' CGPClient = :come_from_cgp'
    expression_attribute_values[':come_from_cgp'] = cgp

    if certificateArn:
        if len(attribute_updates):
            attribute_updates += ', '
        attribute_updates += ' CertificateARN = :certificate_arn'
        expression_attribute_values[':certificate_arn'] = certificateArn

    update_expression = '{} {}'.format('SET',attribute_updates)
    try:
        response_info = client_registry_table.update_item(Key={ 'ClientID': clientId},UpdateExpression=update_expression,ExpressionAttributeValues=expression_attribute_values,ReturnValues='ALL_NEW')
    except Exception as e:
        raise RuntimeError('Could not update status for {} {}'.format(clientId, e.response['Error']['Message']))

    item_data = response_info.get('Attributes', None)

    print 'Registration table update returned {}'.format(item_data)

