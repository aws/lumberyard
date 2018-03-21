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
import json
import time

from cgf_utils import aws_utils
from resource_manager_common import stack_info

from botocore.exceptions import ClientError

iam = aws_utils.ClientWrapper(boto3.client('iam'))

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

PROPAGATION_DELAY_SECONDS = 30

def get_assume_role_policy_document_for_service(service):
    return ASSUME_ROLE_POLICY_DOCUMENT.replace("{assume_role_service}", service)

def create_access_control_role(stack_manager, id_data, stack_arn, logical_role_name, assume_role_service, delay_for_propagation = True, default_policy = None):
    '''Create an IAM Role for a resource group stack. 
    
    Developers typlically do not have the IAM permissions necessary to create
    roles, either directly or indirectly via Cloud Formation. However, many
    resource require that a role be specified (e.g. the role assumed when a
    Lambda function is invoked). 
    
    To overcome this delema, custom resource handlers can use this method and
    act as a proxies for directly defining roles in resource-group-template.json 
    files.
        
    The Custom::AcessControl resource handler will manage the policies attached
    to this role.
    
    Args:
        stack_manager: A stack_info.StackManager() instance that can be used to retrieve cached stack information.

        resource_group_stack_arn: Identifies the stack that "owns" the role.

        logical_role_name: Appended to the stack name to construct the actual role name.

        assume_role_service: Identifies the AWS service that is allowed to assume the role.

        default_policy (named): An IAM policy document that will be attached to the role.

    Returns:

        The ARN of the created role.

    '''

    role_name = get_access_control_role_name(stack_arn, logical_role_name)
    owning_stack_info = stack_manager.get_stack_info(stack_arn)

    deployment_name = "NONE"
    resource_group_name = "NONE"
    project_name = "NONE"

    if owning_stack_info.stack_type == stack_info.StackInfo.STACK_TYPE_RESOURCE_GROUP:
        deployment_name = owning_stack_info.deployment.deployment_name
        resource_group_name = owning_stack_info.resource_group_name
        project_name = owning_stack_info.deployment.project.project_name
    elif owning_stack_info.stack_type == stack_info.StackInfo.STACK_TYPE_DEPLOYMENT:
        deployment_name = owning_stack_info.deployment.deployment_name
        project_name = owning_stack_info.project.project_name
    elif owning_stack_info.stack_type == stack_info.StackInfo.STACK_TYPE_PROJECT:
        project_name = owning_stack_info.project_name

    path = '/{project_name}/{deployment_name}/{resource_group_name}/{logical_role_name}/AccessControl/'.format(
        project_name=project_name,
        deployment_name=deployment_name,
        resource_group_name=resource_group_name,
        logical_role_name=logical_role_name)

    assume_role_policy_document = get_assume_role_policy_document_for_service(assume_role_service)

    try:
        res = iam.create_role(RoleName=role_name, AssumeRolePolicyDocument=assume_role_policy_document, Path=path)
        role_arn = res['Role']['Arn']
    except ClientError as e:
        if e.response["Error"]["Code"] != 'EntityAlreadyExists':
            raise e
        res = iam.get_role(RoleName=role_name)
        role_arn = res['Role']['Arn']

    if default_policy:
        iam.put_role_policy(RoleName=role_name, PolicyName='Default', PolicyDocument=default_policy)

    set_id_data_abstract_role_mapping(id_data, logical_role_name, role_name)

    # Allow time for the role to propagate before lambda tries to assume
    # it, which lambda tries to do when the function is created.
    if delay_for_propagation:
        time.sleep(PROPAGATION_DELAY_SECONDS)

    return role_arn


def delete_access_control_role(id_data, logical_role_name):
    '''Delete a role that was created using create_access_control_role.
    
    Args:

        stack_arn: Identifies the stack.

        logical_role_name: Appended to the stack name to construct the actual role name.

        policy_name: Identifies the metadata used to construct the policy and the name 
        of the policy attached to the created role.
    '''

    role_name = get_id_data_abstract_role_mapping(id_data, logical_role_name)

    # IAM requires that the policies be deleted before the role can be deleted. All
    # the policies other than the "default" policy (if any) should have been deleted
    # by the AccessControl resource handler before the role itself is deleted. But
    # this may not happen if DependsOn isn't set correctly, or for other reasons.
    # So, for robustness, we attempt to delete all the policies before attempting to
    # delete the role.

    policy_names = []
    if role_name is None:
        print "WARNING: could not retrieve role name from mapping, possibly orphaning logical role {}".format(logical_role_name)
    else:
        try:
            res = iam.list_role_policies(RoleName=role_name)
            policy_names = res['PolicyNames']
        except ClientError as e:
            if e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
                raise e

        for policy_name in policy_names:
            try:
                res = iam.delete_role_policy(RoleName=role_name, PolicyName=policy_name)
            except ClientError as e:
                if e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
                    raise e

        try:
            res = iam.delete_role(RoleName=role_name)
        except ClientError as e:
            if e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
                raise e

    remove_id_data_abstract_role_mapping(id_data, logical_role_name)


def get_access_control_role_arn(id_data, logical_role_name):
    '''Get the ARN of a role created using create_access_control_role.
    
    Args:

        resource_group_stack_arn: Identifies the stack.

        logical_role_name: Appended to the stack name to construct the actual role name.

    Returns:

        The ARN of the role.

    '''

    role_name = get_id_data_abstract_role_mapping(id_data, logical_role_name)
    if not role_name:
        raise RuntimeError('No abstract role mapping for {} found in id_data {}.'.format(logical_role_name, id_data))

    res = iam.get_role(RoleName=role_name)
    role_arn = res['Role']['Arn']

    return role_arn


MAX_ROLE_NAME_LENGTH = 64


def sanitize_role_name(role_name):
    result = role_name

    if len(role_name) > MAX_ROLE_NAME_LENGTH:
        digest = "-%x" % (hash(role_name) & 0xffffffff)
        result = role_name[:MAX_ROLE_NAME_LENGTH - len(digest)] + digest

    return result


def get_access_control_role_name(stack_arn, logical_role_name):
    '''Generates a role name based on a stack name and a logical role name.

    Args:

        stack_arn - The arn of the stack.

        logical_role_name: Appended to the stack name to construct the actual role name.

    '''

    # Expecting stack names like: {project-name}-{deployment-name}-{resource-group-name}-{random-stuff}.
    # In practice role names are truncated to the max allowable length.

    stack_name = aws_utils.get_stack_name_from_stack_arn(stack_arn)
    role_name = sanitize_role_name(stack_name + '-' + logical_role_name)

    return role_name

def set_id_data_abstract_role_mapping(id_data, logical_role_name, physical_role_name):
    role_mappings = get_id_data_abstract_role_mappings(id_data)
    role_mappings[logical_role_name] = physical_role_name

def get_id_data_abstract_role_mapping(id_data, logical_role_name):
    role_mappings = get_id_data_abstract_role_mappings(id_data)
    return role_mappings.get(logical_role_name, None)

def remove_id_data_abstract_role_mapping(id_data, logical_role_name):
    role_mappings = get_id_data_abstract_role_mappings(id_data)
    return role_mappings.pop(logical_role_name, None)

def get_id_data_abstract_role_mappings(id_data):
    '''Get the logical and physical names of the access control roles
    defined by a resource group.

    Args:

        id_data - Data extracted from a custom resource's physical id by 
        get_data_from_custom_physical_resource_id 

    Returns:

        A dictonary mapping abstract role names to physical role names.
            
    '''

    return id_data.setdefault('AbstractRoleMappings', {})


