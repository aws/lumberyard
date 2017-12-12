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

import account_utils
import errors
import service

IDP_COGNITO = 'Cognito'

@service.api(logging_filter=account_utils.apply_logging_filter)
def get(request, cognitoUsername):
    user = account_utils.get_user(cognitoUsername)

    return account_utils.convert_user_from_cognito_to_model(user)

@service.api(logging_filter=account_utils.apply_logging_filter)
def put(request, cognitoUsername, UpdateCognitoUser=None):
    updates = []
    if UpdateCognitoUser:
        for key, value in UpdateCognitoUser.iteritems():
            if key in account_utils.COGNITO_ATTRIBUTES:
                updates.append({'Name': key, 'Value': value})
            else:
                raise errors.ClientError('Invalid field {}.'.format(key))

    if updates:
        response = account_utils.get_user_pool_client().admin_update_user_attributes(
            UserPoolId=account_utils.get_user_pool_id(),
            Username=cognitoUsername,
            UserAttributes=updates
        )
        print 'Updated: ', account_utils.logging_filter(updates)
    else:
        print 'No updates provided in the request'

    user = account_utils.get_user(cognitoUsername)
    return account_utils.convert_user_from_cognito_to_model(user)
