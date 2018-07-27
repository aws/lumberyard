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
# $Revision: #1 $

from resource_manager.errors import HandledError
from cgf_utils import aws_utils
import boto3
import json
from resource_manager import util

def after_resource_group_updated(hook, resource_group_uploader,  **kwargs):
    stack_arn = hook.context.config.get_deployment_stack_id(kwargs['deployment_name'])
    deployment_region = util.get_region_from_arn(hook.context.config.project_stack_id)
    stack_resources = hook.context.stack.describe_resources(stack_arn, recursive=True)
    channel_table = stack_resources.get('CloudGemWebCommunicator.ChannelDataTable',{}).get('PhysicalResourceId')
    
    if not channel_table:
        return

    dynamodb = aws_utils.ClientWrapper(boto3.client('dynamodb', region_name=deployment_region))

    aggregate_settings = hook.context.config.aggregate_settings
    if aggregate_settings:
        for gem_name, settings_data in aggregate_settings.iteritems():
            communicator_settings = settings_data.get('GemSettings',{}).get('CloudGemWebCommunicator')
            if not communicator_settings:
                continue

            channel_list = communicator_settings.get('Channels')

            if not channel_list:
                continue

            for this_channel in channel_list:
                attributeUpdate = {
                    'Types': {
                        'Value': {
                            'S': json.dumps(this_channel.get('Types'))
                        }
                    }
                }

                if this_channel.get('CommunicationChannel'):
                    attributeUpdate['CommunicationChannel'] = {
                        'Value': {
                            'S': this_channel.get('CommunicationChannel')
                        }
                    }
                else:
                    attributeUpdate['CommunicationChannel'] = {
                        'Action': 'DELETE'
                    }

                try:
                    response = dynamodb.update_item(
                        TableName=channel_table,
                        Key={
                            'Channel': {
                                'S': this_channel.get('Name')
                            }
                        },
                        AttributeUpdates=attributeUpdate,
                        ReturnValues='ALL_NEW'
                    )
                except Exception as e:
                    raise HandledError(
                        'Could not update status for'.format(
                            Channel=this_channel.get('Name'),
                        ),
                        e
                    )
                item_data = response.get('Attributes', None)




