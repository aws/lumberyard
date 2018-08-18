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

import os
import json
import channel_list
from registration_shared import iot_data

def __get_channel_prefix():
    if not hasattr(__get_channel_prefix,'channel_prefix'):
        lambda_name = os.environ.get('AWS_LAMBDA_FUNCTION_NAME')
        lambda_name_split = lambda_name.split('-')
        __get_channel_prefix.channel_prefix = '{}/{}/'.format(lambda_name_split[0], lambda_name_split[1])
    return __get_channel_prefix.channel_prefix

def __get_message_size_limit():
    # IOT currently has a 5 KB size boundary - messages above 5KB
    # will be broken apart and metered as 2 messages
    # This value can be raised with that in mind
    # Upper boundary is currently 128KB
    # Refer to https://aws.amazon.com/iot-core/pricing/ for latest pricing info
    return 5096
    
def __send_message(publish_channel_name, channel_name, message_text, quality_of_service = 0, user_channel = None):
    publish_channel = __get_channel_prefix() + publish_channel_name

    if user_channel:
        publish_channel += user_channel
    
    message_data = {}
    message_data['Channel'] = channel_name
    message_data['Message'] = message_text
    
    payload_str = json.dumps(message_data)
    if len(payload_str) >= __get_message_size_limit():
        print 'Message size limit exceeded - Channel {} message {}'.format(channel_name, message_text)
        return
        
    response = iot_data.publish(topic=publish_channel, qos=quality_of_service, payload=payload_str)

    print 'Sent message {} on channel {} with response {}'.format(message_text, publish_channel, response)
    return { "status": '{}'.format(response)}

def broadcast_message(channel_name, message_text):
    is_valid, send_channel = channel_list.is_valid_broadcast_channel(channel_name, 'BROADCAST')
    if not is_valid:
        print 'Invalid broadcast request on channel {} message {}'.format(channel_name, message_text)
        return

    return __send_message(send_channel, channel_name, message_text)

def send_direct_message(channel_name, message_text, cognito_id):
    is_valid, send_channel = channel_list.is_valid_broadcast_channel(channel_name, 'PRIVATE')
    if not is_valid:
        print 'Invalid private message request on channel {} message {}'.format(channel_name, message_text)
        return

    user_channel = '/client/{}'.format(cognito_id)
    return __send_message(send_channel, channel_name, message_text, 0, user_channel)