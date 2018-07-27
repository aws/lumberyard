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

import boto3
import json
import CloudCanvas
from cgf_utils import aws_utils
import errors

iot_client = aws_utils.ClientWrapper(boto3.client('iot'))
listener_policy = CloudCanvas.get_setting('IotPlayerPolicy')

def __get_player_channel_list(action_type, filter_type):
    iot_policy = iot_client.get_policy(policyName=listener_policy)

    channel_list = []
    policy_document = json.loads(iot_policy.get('policyDocument', ''))
    for thisStatement in policy_document.get('Statement', []):
        if action_type in thisStatement.get('Action', ''):
            for thisChannel in thisStatement.get('Resource', []):
                subscription = thisChannel.split(filter_type)[1]
                this_item = {}
                this_item['ChannelName'] = subscription.split('/')[2]
                ## If the channel was of a BROADCAST or PRIVATE channel it will appear in the subscription list and the
                ## Client should subscribe to it - to the client though we don't want to say BROADCAST or PRIVATE (since it can't broadcast
                ## So we say "receive" meaning listen - we'll have another designation for send/receive if/when we decide to allow two way by default
                this_item['CommunicationType'] = 'RECEIVE'
                this_item['Subscription'] = subscription
                channel_list.append(this_item)
    return channel_list

def get_subscription_channels():
    if not hasattr(get_subscription_channels, 'channel_list'):
        get_subscription_channels.channel_list = __get_player_channel_list('iot:Subscribe',':topicfilter/')
    return get_subscription_channels.channel_list

def _get_channel_table():
    if not hasattr(_get_channel_table, 'channel_table'):
        channel_table_name = CloudCanvas.get_setting('ChannelDataTable')

        dynamoresource = boto3.resource('dynamodb')
        _get_channel_table.channel_table = dynamoresource.Table(channel_table_name)

        if _get_channel_table.channel_table is None:
            raise errors.PortalRequestError('No Channel Table')

    return _get_channel_table.channel_table

def __build_channel_list():
    print 'Listing channel info'
    resultData = []

    tableinfo = _get_channel_table()
    table_data = tableinfo.scan()
    
    subscription_list = get_subscription_channels()
    
    def append_channel_data(item_data):
        channel_name = item_data.get('Channel')
        communication_type_list = json.loads(item_data.get('Types', '[]'))
        for channel_communication_type in communication_type_list:
            this_result = {}
            this_result['ChannelName'] = channel_name
            this_result['CommunicationType'] = channel_communication_type
            communication_channel = item_data.get('CommunicationChannel')
            if communication_channel:
                this_result['CommunicationChannel'] = communication_channel
            else:
                for subscription_info in subscription_list:
                    this_subscription = subscription_info.get('Subscription', '')
                    parsed_subscription = this_subscription.split('/')
                    subscription_channel_name = parsed_subscription[2]
                    if subscription_channel_name == channel_name:
                        if channel_communication_type == 'BROADCAST':
                            ### Channels are set as /project/deployment/channelname for broadcast
                            ### and /project/deployment/channelname/client/${cognito-identity.amazonaws.com:sub} for private
                            if len(parsed_subscription)==3:
                                this_result['Subscription'] = this_subscription
                        elif channel_communication_type == 'PRIVATE':
                            if len(parsed_subscription) > 3 and parsed_subscription[3] == 'client':
                                this_result['Subscription'] = this_subscription
                                
            print 'Appending Channel Item {}'.format(this_result)
            resultData.append(this_result)

    for item_data in table_data['Items']:
        print 'Found item {}'.format(item_data)
        append_channel_data(item_data)

    while table_data.get('LastEvaluatedKey', None) != None:
        table_data = tableinfo.scan(ExclusiveStartKey=table_data.get('LastEvaluatedKey'))

        for item_data in table_data['Items']:
            print 'Found item {}'.format(item_data)
            append_channel_data(item_data)

    return resultData

def get_full_channel_list():
    if not hasattr(get_full_channel_list, 'channel_list'):
        get_full_channel_list.channel_list = __build_channel_list()
    return get_full_channel_list.channel_list

def __build_broadcast_table():
    channel_table = {}
    channel_list = get_full_channel_list()

    for this_channel in channel_list:
        channel_name = this_channel.get('ChannelName')
        if not channel_name:
            ## Should only happen if data is not filled out correctly, but we don't want an anonymous channel
            continue
        channel_entry = channel_table.get(channel_name, {})
        communication_types = channel_entry.get('CommunicationType', [])

        channel_communication_type = this_channel.get('CommunicationType')

        if not channel_communication_type:
            continue

        if not channel_communication_type in communication_types:
            communication_types.append(channel_communication_type)

        channel_entry['CommunicationType'] = communication_types
        if this_channel.get('CommunicationChannel'):
            channel_entry['CommunicationChannel'] = this_channel.get('CommunicationChannel')

        channel_table[channel_name] = channel_entry
    return channel_table

def get_broadcast_table():
    if not hasattr(get_broadcast_table, 'channel_table'):
        get_broadcast_table.channel_table = __build_broadcast_table()
    return get_broadcast_table.channel_table

#return Pair of valid, broadcastChannel
def is_valid_broadcast_channel(channelName, communicationType):
    broadcast_table = get_broadcast_table()

    channel_info = broadcast_table.get(channelName)

    if not channel_info:
        return False, None

    return_channel = channel_info.get('CommunicationChannel', channelName)
    return communicationType in channel_info.get('CommunicationType', []), return_channel

