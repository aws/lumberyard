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

import json

from cgf_utils import custom_resource_utils
from resource_manager import util
from resource_manager.errors import HandledError
from six import iteritems


def after_resource_group_updated(hook, resource_group_uploader, **kwargs):
    stack_arn = hook.context.config.get_deployment_stack_id(kwargs['deployment_name'])
    deployment_region = util.get_region_from_arn(hook.context.config.project_stack_id)
    stack_resources = hook.context.stack.describe_resources(stack_arn, recursive=True)
    channel_table_id = stack_resources.get('CloudGemWebCommunicator.ChannelDataTable', {}).get('PhysicalResourceId')

    if not channel_table_id:
        return

    channel_table_id = custom_resource_utils.get_embedded_physical_id(channel_table_id)

    dynamo_resource = hook.context.aws.resource('dynamodb', region=deployment_region)
    channel_table = dynamo_resource.Table(channel_table_id)

    if not channel_table:
        raise HandledError('Could not get table {}'.format(channel_table_id))

    response = channel_table.scan()
    items = response['Items']

    updated_channels = []
    aggregate_settings = hook.context.config.aggregate_settings
    if aggregate_settings:
        for gem_name, settings_data in iteritems(aggregate_settings):
            communicator_settings = settings_data.get('GemSettings', {}).get('CloudGemWebCommunicator')
            if not communicator_settings:
                continue

            channel_list = communicator_settings.get('Channels')

            if not channel_list:
                continue

            for this_channel in channel_list:
                updated_channels.append(this_channel.get('Name'))

                update_expression = 'SET Types = :types'
                expression_attribute_values = {}
                expression_attribute_values[':types'] = json.dumps(this_channel.get('Types'))
                channel_update = None

                if this_channel.get('CommunicationChannel'):
                    update_expression += ', CommunicationChannel = :communicationChannel'
                    expression_attribute_values[':communicationChannel'] = this_channel.get('CommunicationChannel')

                else:
                    channel_update = 'REMOVE CommunicationChannel'

                try:
                    response = channel_table.update_item(
                        Key={'Channel': this_channel.get('Name')},
                        UpdateExpression=update_expression,
                        ExpressionAttributeValues=expression_attribute_values,
                        ReturnValues='ALL_NEW'
                    )
                except Exception as e:
                    raise HandledError(
                        'Could not update status for channel {} in table {} update_expression {} : {}'.format(
                            this_channel.get('Name', 'NULL'), channel_table_id, update_expression, str(e)
                        )
                    )

                if channel_update:
                    try:
                        response = channel_table.update_item(
                            Key={'Channel': this_channel.get('Name')},
                            UpdateExpression=channel_update,
                            ReturnValues='ALL_NEW'
                        )
                    except Exception as e:
                        raise HandledError(
                            'Could not update status for channel {} in table {} channel_update {} : {}'.format(
                                this_channel.get('Name', 'NULL'), channel_table_id, channel_update, str(e)
                            )
                        )

                item_data = response.get('Attributes', None)

    for this_item in items:
        channel_name = this_item.get('Channel')
        if channel_name not in updated_channels:
            try:
                response = channel_table.delete_item(
                    Key={
                        'Channel': channel_name
                    }
                )
            except Exception as e:
                raise HandledError(
                    'Could not delete entry for {} : {}'.format(channel_name, str(e))
                )
