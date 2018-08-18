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

import service
import admin_messages
import datetime

GEM_NAME = 'CloudGemMessageOfTheDay'

@service.api
def post(request, test_data):
    print("Message of the Day load test clean up entered")
    print(test_data)

    active_messages = test_data['active_message_ids']
    inactive_messages = test_data['inactive_message_ids']

    for messageID in inactive_messages:
        admin_messages.delete(None, messageID)

    for messageID in active_messages:
        admin_messages.delete(None, messageID)

    return "success"
