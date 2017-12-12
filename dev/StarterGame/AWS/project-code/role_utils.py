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
# $Revision$

import boto3
import discovery_utils
import json
import time
import copy

from errors import ValidationError
from botocore.exceptions import ClientError

iam = boto3.client('iam')

def _default_policy_metadata_filter(entry):
    return True # include

def create_role(stack_arn, logical_role_name, policy_name, assume_role_service, default_statements = [], policy_metadata_filter = _default_policy_metadata_filter):
    '''Create a role with a policy constructed using metadata found on a stack's resources.
    
    Args:

        stack_arn: Identifies the stack.

        logical_role_name: Appended to the stack name to construct the actual role name.

        policy_name: Identifies the metadata used to construct the policy and the name 
        of the policy attached to the created role.

        assume_role_service: Identifies the AWS service that is allowed to assume the role.

        default_statements (named): A list containing additional statements included in 
        the policy. Default is [].  

        policy_metadata_filter (name): A function that takes a single parameter and returns 
        True if the provided value should be used. The default function always returns True.

    Returns:

        The ARN of the created role.

    Note:

        The policy attached to the role is constructed by looking in the CloudCanvas metadata
        object for a property identified by the policy_name parameter on each of the resources 
        defined by the specified stack. The value of this property can be an object with an
        "Action" property, or it can be a list of such objects. The value, or each value in 
        the list of values, is passed to the policy_metadata_filter function to determine if
        it should be used in the construction of the policy. An error occurs if the filter 
        accepts more than one value.
    '''

    role_name = _get_role_name(stack_arn, logical_role_name)

    resource_group = discovery_utils.ResourceGroupInfo(stack_arn)
    path = '/{project_name}/{deployment_name}/{resource_group_name}/{logical_role_name}/'.format(
        project_name=resource_group.deployment.project.project_name,
        deployment_name=resource_group.deployment.deployment_name,
        resource_group_name=resource_group.resource_group_name,
        logical_role_name=logical_role_name)

    assume_role_policy_document = ASSUME_ROLE_POLICY_DOCUMENT.replace('{assume_role_service}', assume_role_service)

    print 'create_role {} with path {}'.format(role_name,path)
    res = iam.create_role(RoleName=role_name, AssumeRolePolicyDocument=assume_role_policy_document, Path=path)
    print 'create_role {} result: {}'.format(role_name, res)

    role_arn = res['Role']['Arn']

    _set_role_policy(role_name, stack_arn, policy_name, default_statements, policy_metadata_filter)

    # Allow time for the role to propagate before lambda tries to assume 
    # it, which lambda tries to do when the function is created.
    time.sleep(30) 

    return role_arn


def update_role(stack_arn, logical_role_name, policy_name, default_statements = [], policy_metadata_filter = _default_policy_metadata_filter):
    '''Update a role with a policy that is constructed using metadata found on a stack's resources.
    
    Args:

        stack_arn: Identifies the stack.

        logical_role_name: Appended to the stack name to construct the actual role name.

        policy_name: Identifies the metadata used to construct the policy and the name 
        of the policy attached to the created role.

        default_statements (named): A list containing additional statements included in 
        the policy. Default is [].  

        policy_metadata_filter (name): A function that takes a single parameter and returns 
        True if the provided value should be used. The default function always returns True.

    Returns:

        The ARN of the created role.

    Note:

        See the documentation of the create_role function for a description of how the 
        policy attached to the role is constructed.
    '''

    role_name = _get_role_name(stack_arn, logical_role_name)

    res = iam.get_role(RoleName=role_name)
    print 'get_role {} result: {}'.format(role_name, res)

    role_arn = res['Role']['Arn']

    _set_role_policy(role_name, stack_arn, policy_name, default_statements, policy_metadata_filter)

    return role_arn


def delete_role(stack_arn, logical_role_name, policy_name):
    '''Delete a role with a policy that was constructed using metadata found on a stack's resources.
    
    Args:

        stack_arn: Identifies the stack.

        logical_role_name: Appended to the stack name to construct the actual role name.

        policy_name: Identifies the metadata used to construct the policy and the name 
        of the policy attached to the created role.
    '''

    role_name = _get_role_name(stack_arn, logical_role_name)
    try:
        res = iam.delete_role_policy(RoleName=role_name, PolicyName=policy_name)
        print 'delete_role_policy(RoleName="{}", PolicyName="{}") response: {}'.format(role_name, policy_name, res)
    except Exception as e:
        print 'delete_role_policy(RoleName="{}", PolicyName="{}") error: {}'.format(role_name, policy_name, getattr(e, 'response', e))
        if isinstance(e, ClientError) and e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
            raise e    
        
    try:
        res = iam.delete_role(RoleName=role_name)
        print 'delete_role {} result: {}'.format(role_name, res)
    except Exception as e:
        print 'delete_role {} error: {}'.format(role_name, getattr(e, 'response', e))
        if isinstance(e, ClientError) and e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
            raise e    




ASSUME_ROLE_POLICY_DOCUMENT = '''{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": "sts:AssumeRole",
            "Principal": {
                "Service": "{assume_role_service}"
            }
        }
    ]
}'''


MAX_ROLE_NAME_LENGTH = 64


def _get_role_name(stack_arn, logical_role_name):

    # Expecting stack names like: {project-name}-{deployment-name}-{resource-group-name}-{random-stuff}.
    # We shorten the {project-name}-{deployment-name}-{resource-group-name} part to the point that we
    # can append {random-stuff} and the role's logical name and end up with a name that meets the max
    # role name length requirement.

    stack_name = discovery_utils.get_stack_name_from_stack_arn(stack_arn)

    if len(stack_name) + len(logical_role_name) + 1 <= MAX_ROLE_NAME_LENGTH:

        role_name = stack_name + '-' + logical_role_name

    else:

        last_dash = stack_name.rfind('-')
        if last_dash == -1:
            raise ValidationError('Stack name does not have the expected format: {}'.format(stack_name))

        root_part = stack_name[:last_dash]
        random_part = stack_name[last_dash+1:]

        root_part_len = MAX_ROLE_NAME_LENGTH - (len(random_part) + len(logical_role_name) + 2)
        if root_part_len < 0:
            raise ValidationError('The logical role name is too long: {}'.format(logical_role_name))

        root_part = root_part[:root_part_len]

        role_name = root_part + '-' + random_part + '-' + logical_role_name
    
    return role_name


def _set_role_policy(role_name, stack_arn, policy_name, default_statements, policy_metadata_filter):

    policy = _create_role_policy(stack_arn, policy_name, default_statements, policy_metadata_filter)

    if policy is None:
        try:
            res = iam.delete_role_policy(RoleName=role_name, PolicyName=policy_name)
            print 'delete_role_policy(RoleName="{}", PolicyName="{}") response: {}'.format(role_name, policy_name, res)
        except Exception as e:
            print 'delete_role_policy(RoleName="{}", PolicyName="{}") error: {}'.format(role_name, policy_name, getattr(e, 'response', e))
            if isinstance(e, ClientError) and e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
                raise e    
    else:
        res = iam.put_role_policy(RoleName=role_name, PolicyName=policy_name, PolicyDocument=policy)
        print 'put_role_policy {} on role {} result: {}'.format(policy_name, role_name, res)


def _create_role_policy(stack_arn, policy_name, default_statements, policy_metadata_filter):

    if not isinstance(default_statements, list):
        raise ValidationError('The default_statements value is not a list.')

    cf = boto3.client('cloudformation', region_name=discovery_utils.get_region_from_stack_arn(stack_arn))

    try:
        res = discovery_utils.try_with_backoff(lambda : cf.describe_stack_resources(StackName=stack_arn))
        print 'describe_stack_resource(StackName="{}") result: {}'.format(stack_arn, res)
    except Exception as e:
        print 'describe_stack_resource(StackName="{}") error: {}'.format(stack_arn, getattr(e, 'response', e))
        raise e

    policy = {
        'Version': '2012-10-17',
        'Statement': copy.deepcopy(default_statements)
    }

    for resource in res['StackResources']:
        statement = _make_resource_statement(cf, stack_arn, policy_name, resource['LogicalResourceId'], policy_metadata_filter)
        if statement is not None:
            policy['Statement'].append(statement)
        
    print 'generated policy: {}'.format(policy)

    if len(policy['Statement']) == 0:
        return None
    else:
        return json.dumps(policy, indent=4)


def _make_resource_statement(cf, stack_arn, policy_name, logical_resource_name, policy_metadata_filter):

    print 'describe_stack_resource on resource {} in stack {}.'.format(logical_resource_name, stack_arn)
    try:
        res = discovery_utils.try_with_backoff(lambda : cf.describe_stack_resource(StackName=stack_arn, LogicalResourceId=logical_resource_name))
        print 'describe_stack_resource {} result: {}'.format(logical_resource_name, res)
    except Exception as e:
        print 'describe_stack_resource {} error: {}'.format(logical_resource_name, getattr(e, 'response', e))
        raise e
    resource = res['StackResourceDetail']

    metadata = _get_metadata_for_role(resource, policy_name, policy_metadata_filter)
    if metadata is None: 
        return

    metadata_actions = metadata.get('Action', None)
    if metadata_actions is None:
        raise ValidationError('No Action was specified for CloudCanvas {} metdata on the {} resource in stack {}.'.format(
            policy_name,
            resource['LogicalResourceId'], 
            stack_arn))

    if not isinstance(metadata_actions, list):
        metadata_actions = [ metadata_actions ]

    for action in metadata_actions:
        if not isinstance(action, basestring):
            raise ValidationError('Non-string Action specified for CloudCanvas {} metadata on the {} resource in stack {}.'.format(
                policy_name,
                resource['LogicalResourceId'], 
                stack_arn))

    resource = discovery_utils.get_resource_arn(stack_arn, resource['ResourceType'], resource['PhysicalResourceId'])

    resource_suffix = metadata.get('ResourceSuffix', None) 
    if resource_suffix is not None:
        resource += resource_suffix

    return {
        'Sid': logical_resource_name + 'Access',
        'Effect': 'Allow',
        'Action': metadata_actions,
        'Resource': resource
    }


def _get_metadata_for_role(resource, policy_name, policy_metadata_filter):
    
    metadata = discovery_utils.get_cloud_canvas_metadata(resource, policy_name)

    if metadata is None:
        return None

    if isinstance(metadata, dict):
        metadata = [ metadata ]

    if not isinstance(metadata, list):
        raise ValidationError('{} metadata not an object or list on resource {} in stack {}.'.format(
            policy_name,
            resource['LogicalResourceId'],
            stack_arn))

    entry_found = None

    for entry in metadata:

        try:
            entry_accepted = policy_metadata_filter(entry)
        except ValidationError as e:
            raise ValidationError('Invalid {} metadata entry was found on resource {} in stack {}. {}'.format(
                policy_name,
                resource['LogicalResourceId'],
                stack_arn, 
                e.message))

        if entry_accepted:
            if entry_found is not None:
                raise ValidationError('More than one applicable {} metadata entry was found on resource {} in stack {}.'.format(
                    policy_name,
                    resource['LogicalResourceId'],
                    stack_arn))
            entry_found = entry

    return entry_found

