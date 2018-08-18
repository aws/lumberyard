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

import service
import messaging

@service.api
def post(request, message_request, cognito_id):
    print 'Got send direct request: {}'.format(request)
    print 'Cognito ID {} Message request: {}'.format(cognito_id,message_request)  
    return messaging.send_direct_message(message_request['channel'], message_request['message'], cognito_id)