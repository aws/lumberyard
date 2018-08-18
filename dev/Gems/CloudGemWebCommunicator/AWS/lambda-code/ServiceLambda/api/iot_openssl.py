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
import registration_shared
from datetime import datetime
from random import randrange

from registration_shared import client_registry_table
from registration_shared import iot_client
from OpenSSL.crypto import load_certificate, FILETYPE_PEM

def _create_keys_and_certificate():

    keys_and_cert = iot_client.create_keys_and_certificate(setAsActive=True)

    return keys_and_cert


def _create_thing(cognitoId, thingName):

    print 'Attempting to create thing {}'.format(thingName)

    thingResponse = iot_client.create_thing(thingName=thingName)
    
    _set_device_info(cognitoId, thingResponse['thingName'], thingResponse['thingArn'])
    
    print 'Created thing Name {} arn {}'.format(thingResponse['thingName'], thingResponse['thingArn'])

    return thingResponse


def _set_device_info(cognitoId, thingName, thingArn):
    
    print 'Setting thing for user {} to {} arn {}'.format(cognitoId, thingName, thingArn)
    
    attribute_updates = ''
    expression_attribute_values = {}
    if thingName is not None:
        attribute_updates += ' ThingName = :thingName'
        expression_attribute_values[':thingName'] = thingName         
        
    if len(attribute_updates):
        attribute_updates += ', '
    attribute_updates += ' ThingArn = :thingArn'
    expression_attribute_values[':thingArn'] = thingArn    
    
    update_expression = '{} {}'.format('SET',attribute_updates)
    try:
        response_info = client_registry_table.update_item(Key={ 'ClientID': cognitoId},UpdateExpression=update_expression,ExpressionAttributeValues=expression_attribute_values,ReturnValues='ALL_NEW')
    except Exception as e:
        raise RuntimeError('Could not update thing for {} {}'.format(cognitoId, e.response['Error']['Message']))

    item_data = response_info.get('Attributes', None)

    print 'Registration table device info update returned {}'.format(item_data)


def register_openssl(request):
    clientId = request.event.get('clientId')

    if clientId:
        print 'Attempting openssl registration for id {}'.format(clientId)
        client_info = registration_shared.get_user_entry(clientId)
    else:
        print 'Attempting openssl registration for new user'
        client_info = {}

    registration_status = client_info.get('RegistrationStatus')

    print 'OpenSSL User status for {} returns {}'.format(clientId, registration_status)
    
    responseObject = {}

    if registration_status == 'BANNED':
        responseObject['Result'] = 'DENIED'
        return responseObject
        
    if registration_status == 'UNKNOWN':
        print 'Re-registering user with unknown status {}'.format(clientId)
        
    keys_and_cert = _create_keys_and_certificate()

    certificate_info = load_certificate(FILETYPE_PEM, keys_and_cert['certificatePem'])
    certificate_sn = '{}'.format(certificate_info.get_serial_number())

    print 'Got certificate SN {}'.format(certificate_sn)

    if registration_status in ['NEW_USER', None]:
        registration_shared.create_user_entry(certificate_sn, 'REGISTERED', False, keys_and_cert['certificateArn'])

    print 'Attaching principal {} to policy {}'.format(keys_and_cert['certificateArn'], registration_shared.device_policy)
    registration_shared.check_add_policy(keys_and_cert['certificateArn'], registration_shared.device_policy)

    responseObject['Result'] = 'SUCCESS'
    responseObject['ConnectionType'] = 'OPENSSL'
    
    responseObject['PrivateKey'] = keys_and_cert['keyPair']['PrivateKey']
    responseObject['DeviceCert'] = keys_and_cert['certificatePem']

    return responseObject





 
