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
# $Revision: #6 $

import properties
import custom_resource_response
import boto3
import json
import time

import discovery_utils

from botocore.exceptions import ClientError
from errors import ValidationError

iam = boto3.client('iam')

def handler(event, context):
    
    props = properties.load(event, {
        'ConfigurationBucket': properties.String(), # Currently not used
        'ConfigurationKey': properties.String(),    # Depend on unique upload id in key to force Cloud Formation to call handler
        'RoleLogicalId': properties.String(),
        'MetadataKey': properties.String(),
        'PhysicalResourceId': properties.String(),
        'UsePropagationDelay': properties.String(),
        'RequireRoleExists': properties.String(default='true'),
        'ResourceGroupStack': properties.String(default=''),
        'DeploymentStack': properties.String(default='')})

    if props.ResourceGroupStack is '' and props.DeploymentStack is '':
        raise ValidationError('A value for the ResourceGroupStack property or the DeploymentStack property must be provided.')

    if props.ResourceGroupStack is not '' and props.DeploymentStack is not '':
        raise ValidationError('A value for only the ResourceGroupStack property or the DeploymentStack property can be provided.')

    use_propagation_delay = props.UsePropagationDelay.lower() == 'true'
    
    data = {}
    stack_infos = []
    
    if props.ResourceGroupStack is not '':
        resource_group_info = discovery_utils.ResourceGroupInfo(props.ResourceGroupStack)
        
        # create a list of stack-infos, starting at the resource group level and working our way upward
        stack_infos = _build_stack_infos_list(resource_group_info) 
        
    else: # DeploymentStack
        deployment_info = discovery_utils.DeploymentInfo(props.DeploymentStack)
        
        # create a list of stack-infos, starting at the deployment level and working our way upward
        stack_infos = _build_stack_infos_list(deployment_info)
    
    # go through each of the stack infos, trying to find the specified role
    for stack_info in stack_infos:
        role = stack_info.get_resource(props.RoleLogicalId, expected_type='AWS::IAM::Role', optional=True)
        
        if role is not None:
            break

    role_physical_id = None
    if role is not None:
        role_physical_id = role.get('PhysicalResourceId', None)

    if role_physical_id is None:
        if props.RequireRoleExists.lower() == 'true':
            raise ValidationError('Could not find role \'{}\'.'.format(props.RoleLogicalId))
    else:
        if type(stack_infos[0]) is discovery_utils.ResourceGroupInfo:
            _process_resource_group_stack(event['RequestType'], stack_infos[0], role_physical_id, props.MetadataKey, use_propagation_delay)
        else:
            for resource_group_info in stack_infos[0].get_resource_group_infos():
                _process_resource_group_stack(event['RequestType'], resource_group_info, role_physical_id, props.MetadataKey, use_propagation_delay)

    custom_resource_response.succeed(event, context, data, props.PhysicalResourceId)

def _build_stack_infos_list(first_stack_info):
    stack_infos = []
    deployment_info = None

    if type(first_stack_info) is discovery_utils.ResourceGroupInfo:
        stack_infos.append(first_stack_info)
        deployment_info = first_stack_info.deployment
    elif type(first_stack_info) is discovery_utils.DeploymentInfo:
        deployment_info = first_stack_info
    else:
        raise RuntimeError('Argument first_stack_info for function _build_stack_infos_list must either be of type ResourceGroupInfo or DeploymentInfo')

    stack_infos.append(deployment_info)
    
    deployment_access_stack = deployment_info.get_deployment_access_stack_info()
    
    if deployment_access_stack is not None:
        stack_infos.append(deployment_access_stack)
    
    stack_infos.append(deployment_info.project)
    
    return stack_infos
    
def _process_resource_group_stack(request_type, resource_group_info, role_name, metadata_key, use_propagation_delay):

    policy_name = resource_group_info.stack_name

    if request_type == 'Delete':
        _delete_role_policy(role_name, policy_name)
    else: # Update and Delete
        policy_document = _construct_policy_document(resource_group_info, metadata_key)
        if policy_document is None:
            _delete_role_policy(role_name, policy_name)
        else:
            _put_role_policy(role_name, policy_name, policy_document, use_propagation_delay)

def _construct_policy_document(resource_group_info, metadata_key):

    policy_document = {
        'Version': '2012-10-17',
        'Statement': []
    }

    for resource in resource_group_info.get_resources():
        logical_resource_id = resource.get('LogicalResourceId', None)
        if logical_resource_id is not None:
            statement = _make_resource_statement(resource_group_info, logical_resource_id, metadata_key)
            if statement is not None:
                policy_document['Statement'].append(statement)
        
    if len(policy_document['Statement']) == 0:
        return None

    print 'constructed policy: {}'.format(policy_document)

    return json.dumps(policy_document, indent=4)


def _make_resource_statement(resource_group_info, logical_resource_name, metadata_key):

    try:
        response = discovery_utils.try_with_backoff(lambda : resource_group_info.get_client().describe_stack_resource(StackName=resource_group_info.stack_arn, LogicalResourceId=logical_resource_name))
        print 'describe_stack_resource(LogicalResourceId="{}", StackName="{}") response: {}'.format(logical_resource_name, resource_group_info.stack_arn, response)
    except Exception as e:
        print 'describe_stack_resource(LogicalResourceId="{}", StackName="{}") error: {}'.format(logical_resource_name, resource_group_info.stack_arn, getattr(e, 'response', e))
        raise e

    resource = response['StackResourceDetail']

    metadata = discovery_utils.get_cloud_canvas_metadata(resource, metadata_key)
    if metadata is None:
        return None

    metadata_actions = metadata.get('Action', None)
    if metadata_actions is None:
        raise ValidationError('No Action was specified for CloudCanvas Access metdata on the {} resource in stack {}.'.format(
            logical_resource_name, 
            resource_group_info.stack_arn))
    if not isinstance(metadata_actions, list):
        metadata_actions = [ metadata_actions ]
    for action in metadata_actions:
        if not isinstance(action, basestring):
            raise ValidationError('Non-string Action specified for CloudCanvas Access metadata on the {} resource in stack {}.'.format(
                logical_resource_name, 
                resource_group_info.stack_arn))

    if 'PhysicalResourceId' not in resource:
        return None

    if 'ResourceType' not in resource:
        return None

    resource = discovery_utils.get_resource_arn(resource_group_info.stack_arn, resource['ResourceType'], resource['PhysicalResourceId'])

    resource_suffix = metadata.get('ResourceSuffix', None) 
    if resource_suffix is not None:
        resource += resource_suffix

    return {
        'Sid': logical_resource_name + 'Access',
        'Effect': 'Allow',
        'Action': metadata_actions,
        'Resource': resource
    }


def _put_role_policy(role_name, policy_name, policy_document, use_propagation_delay):
    try:
        response = iam.put_role_policy(RoleName=role_name, PolicyName=policy_name, PolicyDocument=policy_document)
        print 'put_role_policy(RoleName="{}", PolicyName="{}", PolicyDocument="{}") response: {}'.format(role_name, policy_name, policy_document, response)
        
        if use_propagation_delay == True:
            # Allow time for the role to propagate before lambda tries to assume 
            # it, which lambda tries to do when the function is created.
            time.sleep(60) 
        
    except Exception as e:
        print 'put_role_policy(RoleName="{}", PolicyName="{}", PolicyDocument="{}") error: {}'.format(role_name, policy_name, policy_document, getattr(e, 'response', e))
        raise e


def _delete_role_policy(role_name, policy_name):
    try:
        response = iam.delete_role_policy(RoleName=role_name, PolicyName=policy_name)
        print 'delete_role_policy(RoleName="{}", PolicyName="{}") response: {}'.format(role_name, policy_name, response)
    except Exception as e:
        print 'delete_role_policy(RoleName="{}", PolicyName="{}") error: {}'.format(role_name, policy_name, getattr(e, 'response', e))
        if isinstance(e, ClientError) and e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
            raise e


