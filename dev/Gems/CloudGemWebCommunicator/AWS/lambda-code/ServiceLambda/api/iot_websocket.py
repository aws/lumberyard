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

def register_websocket(request, cgp = False):
    cognitoId = request.event.get('cognitoIdentityId')
    cognitoIdentityPoolId = request.event.get('cognitoIdentityPoolId')

    responseObject = {}

    print 'Attempting websocket registration for cognitoId {} PoolId {}'.format(cognitoId, cognitoIdentityPoolId)

    client_info = registration_shared.get_user_entry(cognitoId)

    registration_status = client_info.get('RegistrationStatus')

    print 'User status for {} returns {}'.format(cognitoId, registration_status)

    if registration_status == 'BANNED':
        responseObject['Result'] = 'DENIED'
        return responseObject
    elif registration_status in ['NEW_USER', None]:
        registration_shared.create_user_entry(cognitoId, 'REGISTERED', cgp)
    elif registration_status == 'UNKNOWN':
        print 'Re-registering user with unknown status {}'.format(cognitoId)
        registration_shared.create_user_entry(cognitoId, 'REGISTERED', cgp)

    if cgp:
        registration_shared.check_add_policy(cognitoId, registration_shared.cgp_listener_policy)
    else:
        registration_shared.check_add_policy(cognitoId, registration_shared.listener_policy)

    responseObject['Result'] = 'SUCCESS'
    responseObject['ConnectionType'] = 'WEBSOCKET'

    return responseObject
