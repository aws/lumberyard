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

from cgf_utils import aws_utils
from resource_manager_common import stack_info
import boto3
import json

iot_client = aws_utils.ClientWrapper(boto3.client('iot'))


def _get_subscription_resources(stack, client_id):
    # Custom policy built from resources will come from here

    policy_list = []
    if stack.stack_type == stack_info.StackInfo.STACK_TYPE_RESOURCE_GROUP:
        account_id = aws_utils.ClientWrapper(boto3.client('sts')).get_caller_identity()['Account']

        resource_group_name = stack.resource_group_name
        deployment_name = stack.deployment.deployment_name
        project_name = stack.deployment.parent_stack.project_name

        resource_group_settings = stack.deployment.get_gem_settings(resource_group_name)

        for gem_name, gem_settings in resource_group_settings.iteritems():
            channel_list = gem_settings.get('Channels', [])
            for requested_channel in channel_list:
                requested_name = requested_channel.get('Name')
                if not requested_name:
                    continue
                for requested_type in requested_channel.get('Types', []):
                    iot_client_resource = 'arn:aws:iot:{}:{}:topic/{}/{}/{}'.format(stack.region, account_id,project_name,deployment_name,requested_name)
                    if requested_channel.get('CommunicationChannel', None) != None:
                        # This channel communicates within another channel - We don't want to subscribe to it
                        continue
                    if requested_type == 'PRIVATE':
                        iot_client_resource += '/client/{}'.format(client_id)
                    policy_list.append(iot_client_resource)

    return policy_list

def get_listener_policy(stack, client_id):
    account_id = aws_utils.ClientWrapper(boto3.client('sts')).get_caller_identity()['Account']

    iot_client_resource = "arn:aws:iot:{}:{}:client/{}".format(stack.region, account_id, client_id)

    policy_doc = {}
    policy_doc['Version'] = '2012-10-17'

    policy_list = []
    connect_statement = {}
    connect_statement['Effect'] = 'Allow'
    connect_statement['Action'] = ['iot:Connect']
    connect_statement['Resource'] = "arn:aws:iot:{}:{}:client/{}".format(stack.region, account_id, client_id)
    policy_list.append(connect_statement)

    receive_statement = {}
    receive_statement['Effect'] = 'Allow'
    receive_statement['Action'] = ['iot:Receive']
    receive_list = _get_subscription_resources(stack, client_id)
    receive_statement['Resource'] = receive_list
    policy_list.append(receive_statement)

    subscribe_statement = {}
    subscribe_statement['Effect'] = 'Allow'
    subscribe_statement['Action'] = ['iot:Subscribe']
    subscribe_list = [ channel.replace(":topic/", ":topicfilter/") for channel in receive_list]
    subscribe_statement['Resource'] = subscribe_list
    policy_list.append(subscribe_statement)

    policy_doc['Statement'] = policy_list

    return json.dumps(policy_doc)

def detach_policy_principals(physical_resource_id):
    try:
        principal_list = iot_client.list_policy_principals(policyName=physical_resource_id, pageSize=100)
    except Exception as e:
        # Wrapper should have logged error. Return gracefully rather than raising
        return e
        
    next_marker = principal_list.get('nextMarker')
    while True:
        for thisPrincipal in principal_list['principals']:
            if ':cert/' in thisPrincipal:
                # For a cert, the principal is the full arn
                principal_name = thisPrincipal
            else:
                ## Response is in the form of accountId:CognitoId - when we detach we only want cognitoId
                principal_name = thisPrincipal.split(':',1)[1]
            try:
                iot_client.detach_principal_policy(policyName=physical_resource_id, principal=principal_name)
            except Exception as e:
                return e

        if next_marker is None:
            break
        principal_list = iot_client.list_policy_principals(policyName=physical_resource_id, pageSize=100, marker=next_marker)  
        next_marker = principal_list.get('nextMarker')

    
        
