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

@service.api(logging_filter=account_utils.apply_logging_filter)
def get(request, IdentityProviderId):
    if IdentityProviderId != account_utils.IDP_COGNITO:
        raise errors.ClientError('Only the {} identity provider is supported.'.format(account_utils.IDP_COGNITO))

    pool = account_utils.get_user_pool_client().describe_user_pool(UserPoolId = account_utils.get_user_pool_id())
    numberOfUsers = pool.get('UserPool', {}).get('EstimatedNumberOfUsers', 0)

    return {'EstimatedNumberOfUsers': numberOfUsers}
