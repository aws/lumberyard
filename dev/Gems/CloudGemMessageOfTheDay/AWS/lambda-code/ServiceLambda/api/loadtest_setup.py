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

from __future__ import print_function


import datetime
import service

import admin_messages

GEM_NAME = 'CloudGemMessageOfTheDay'
ACTIVE_MESSAGE_COUNT = 2
INACTIVE_MESSAGE_COUNT = 200
MESSAGE_TEXT_LENGTH = 100

@service.api
def post(request):
    print("MotD load test initialization entered.")

    active_messages_response = admin_messages.get(None, None, 100, 'active')

    active_messages = active_messages_response['list']

    out = {'active_message_ids':[], 'inactive_message_ids':[]}

    if len(active_messages) < ACTIVE_MESSAGE_COUNT:
        print('Adding active messages to {}'.format(GEM_NAME))
        message = {
            'message': 'x' * MESSAGE_TEXT_LENGTH,
            'priority': 1
        }
        for _ in range(ACTIVE_MESSAGE_COUNT - len(active_messages)):
            result = admin_messages.post(None, message)
            out['active_message_ids'].append(result['UniqueMsgID'])

    inactive_messages_response = admin_messages.get(None, None, 300, 'expired')
    inactive_messages = inactive_messages_response['list']

    if len(inactive_messages) < INACTIVE_MESSAGE_COUNT:
        print('Adding inactive messages to {}'.format(GEM_NAME))
        end_time = datetime.datetime.now() - datetime.timedelta(days=10)
        start_time = end_time - datetime.timedelta(days=10)
        message = {
            'message': 'x' * MESSAGE_TEXT_LENGTH,
            'priority': 1,
            'startTime': start_time.strftime('%b %d %Y %H:%M'),
            'endTime': end_time.strftime('%b %d %Y %H:%M')
        }
        for _ in range(INACTIVE_MESSAGE_COUNT - len(inactive_messages)):
            result = admin_messages.post(None, message)
            out['inactive_message_ids'].append(result['UniqueMsgID'])

    return out