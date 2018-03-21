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

from cgf_utils import properties
from cgf_utils import custom_resource_response
import boto3
import json
import time

from cgf_utils import aws_utils
from cgf_utils import role_utils
from resource_manager_common import stack_info

from botocore.exceptions import ClientError

PROPAGATION_DELAY_SECONDS = 10

iam = aws_utils.ClientWrapper(boto3.client('iam'))


class ProblemList(object):

    def __init__(self):
        self.__problems = []
        self.__prefixes = []

    def __repr__(self):
        return '\n    '.join(self.__problems)

    def __nonzero__(self):
        return len(self.__problems) > 0

    def __len__(self):
        return len(self.__problems)

    def push_prefix(self, format, *args):
        self.__prefixes.append((format, args))

    def pop_prefix(self):
        self.__prefixes.pop()

    def append(self, message):
        
        full_message = ''
        for prefix in self.__prefixes:
            full_message = full_message + prefix[0].format(*prefix[1])

        full_message = full_message + message

        self.__problems.append(full_message)


def handler(event, context):
    '''Entry point for the Custom::AccessControl resource handler.'''
    
    props = properties.load(event, {
        'ConfigurationBucket': properties.String(), # Currently not used
        'ConfigurationKey': properties.String()})   # Depend on unique upload id in key to force Cloud Formation to call handler

    # Validate RequestType
    request_type = event['RequestType']
    if request_type not in ['Create', 'Update', 'Delete']:
        raise RuntimeError('Unexpected request type: {}'.format(request_type))

    # Get stack_info for the AccessControl resource's stack.
    stack_arn = event['StackId']
    stack_manager = stack_info.StackInfoManager()
    stack = stack_manager.get_stack_info(stack_arn)

    # Physical ID is always the same.
    physical_resource_id = aws_utils.get_stack_name_from_stack_arn(stack_arn) + '-' + event['LogicalResourceId']

    # The AccessControl resource has no output values. 
    data = {}

    # Accumlate problems encountered so we can give a full report.
    problems = ProblemList()

    # Apply access control as determined by the Cloud Canvas stack type.
    if stack.stack_type == stack.STACK_TYPE_RESOURCE_GROUP:
        were_changes = _apply_resource_group_access_control(request_type, stack, problems)
    elif stack.stack_type == stack.STACK_TYPE_DEPLOYMENT_ACCESS:
        were_changes = _apply_deployment_access_control(request_type, stack, problems)
    elif stack.stack_type == stack.STACK_TYPE_PROJECT:
        were_changes = _apply_project_access_control(request_type, stack, problems)
    else:
        raise RuntimeError('The Custom::AccessControl resource can only be used in resource group, deployment access, or project stack templates.')

    # If there were any problems, provide an error message with all the details.
    if problems:
        raise RuntimeError('Found invalid AccessControl metadata:\n    {}'.format(problems))

    # If there were changes, wait a few seconds for them to propagate
    if were_changes:
        print 'Delaying {} seconds for change propagation'.format(PROPAGATION_DELAY_SECONDS)
        time.sleep(PROPAGATION_DELAY_SECONDS)
    
    # Successful execution.
    return custom_resource_response.success_response(data, physical_resource_id)


def _apply_resource_group_access_control(request_type, resource_group, problems):
    '''Performs the processing necessary when the Custom::AccessControl resource is in a resource group stack.

    Args:

        request_type - 'Create', 'Update', or 'Delete'

        resource_group - an stack_info.ResourceGroupInfo object representing the resource group stack that contains the Custom::AccessControl resource.

        problems - A ProblemList object used to report problems

    '''

    print 'Applying access control {} for resource group stack {}.'.format(request_type, resource_group.stack_name)

    policy_name = _get_resource_group_policy_name(resource_group)

    # Get the permissions desired by the resource group's resources.
    permissions = _get_permissions(resource_group, problems)

    # Get abstract to concrete role mappings for each of the target stacks.
    resource_group_role_mappings = _get_implicit_role_mappings(resource_group, problems)
    if resource_group.deployment.deployment_access:
        deployment_access_role_mappings = _get_explicit_role_mappings(resource_group.deployment.deployment_access, problems)
    else:
        deployment_access_role_mappings = None
    project_role_mappings = _get_explicit_role_mappings(resource_group.deployment.project, problems)

    # If there were no problems collecting the metadata, update the roles in the target stacks as needed
    were_changes = False
    if not problems:
        if _update_roles(request_type, policy_name, permissions, resource_group_role_mappings):
            were_changes = True
        if deployment_access_role_mappings:
            if _update_roles(request_type, policy_name, permissions, deployment_access_role_mappings):
                were_changes = True
        if _update_roles(request_type, policy_name, permissions, project_role_mappings):
            were_changes = True

    return were_changes
    

def _apply_deployment_access_control(request_type, deployment_access, problems):
    '''Performs the processing necessary when the Custom::AccessControl resource is in a deployment access stack.

    Args:

        request_type - 'Create', 'Update', or 'Delete'

        deployment_access - an stack_info.ResourceGroupInfo object representing the deployment access stack that contains the Custom::AccessControl resource.

        problems - A ProblemList object used to report problems

    '''
    
    print 'Applying access control {} for deployment access stack {}.'.format(request_type, deployment_access.stack_name)

    # Get abstract to concrete role mappings for the target stack
    role_mappings = _get_explicit_role_mappings(deployment_access, problems)

    # Get the permissions desired by each of the resource group's resources.
    permissions_by_policy_name = {}
    for resource_group in deployment_access.deployment.resource_groups:
        policy_name = _get_resource_group_policy_name(resource_group)
        permissions_by_policy_name[policy_name] = _get_permissions(resource_group, problems)

    # If there were no problems collecting the metadata, update the roles in the target stack as needed
    were_changes = False
    if not problems:
        for policy_name, permissions in permissions_by_policy_name.iteritems():
            if _update_roles(request_type, policy_name, permissions, role_mappings):
                were_changes = True

    return were_changes
    

def _apply_project_access_control(request_type, project, problems):
    '''Performs the processing necessary when the Custom::AccessControl resource is in a project stack.

    Args:

        request_type - 'Create', 'Update', or 'Delete'

        resource_group - an stack_info.ProjectInfo object representing the project stack that contains the Custom::AccessControl resource.

        problems - A ProblemList object used to report problems

    '''

    print 'Applying access control {} for project stack {}.'.format(request_type, project.stack_name)

    # Get abstract to concrete role mappings for the target stack
    explicit_role_mappings = _get_explicit_role_mappings(project, problems)

    # Get the permissions desired by each of the resource group's resources.
    permissions_by_policy_name = {}
    for deployment in project.deployments:
        for resource_group in deployment.resource_groups:
            policy_name = _get_resource_group_policy_name(resource_group)
            permissions_by_policy_name[policy_name] = _get_permissions(resource_group, problems)

    # Get permissions for resources in the project
    #
    policy_name = _get_project_policy_name(project)
    permissions_by_policy_name[policy_name] = _get_permissions(project, problems)

    # Get implicit role mappings for resources in the project
    implicit_role_mappings = _get_implicit_role_mappings(project, problems)

    # If there were no problems collecting the metadata, update the roles in the target stack as needed
    were_changes = False
    if not problems:
        for policy_name, permissions in permissions_by_policy_name.iteritems():
            updated_exp_roles = _update_roles(request_type, policy_name, permissions, explicit_role_mappings)
            updated_imp_roles = _update_roles(request_type, policy_name, permissions, implicit_role_mappings)
            if updated_exp_roles or updated_imp_roles:
                were_changes = True
                
    return were_changes


def _get_resource_group_policy_name(resource_group):
    '''Constructs a policy name for a resource group.

    Args:

        resource_group - a stack_info.ResourceGroupInfo object that represents the resource group.

    Returns:

        A string containing the policy name.

    '''
    return '{}.{}-AccessControl'.format(resource_group.deployment.deployment_name, resource_group.resource_group_name)

def _get_project_policy_name(project):
    '''Constructs a policy name for a project.

    Args:

        project - a stack_info.ProjectInfo object that represents the project.

    Returns:

        A string containing the policy name.

    '''
    return 'PROJECT.{}-AccessControl'.format(project.project_name)


def _get_permissions(resource_info, problems):
    '''Looks for the following metadata in a stack template:
    
          {
            "Resources": {
              "<logical-resource-id>": {
                "Metadata": {                                                 # optional
                  "CloudCanvas": {                                            # optional
                      "Permissions": [                                        # optional list or a single object
                        {
                          "AbstractRole": [ "<abstract-role-name>", ... ],    # required list or single string
                          "Action": [ "<allowed-action>", ... ]               # required list or single string
                          "ResourceSuffix": [ "<resource-suffix>", ... ]      # required list or single string
                        },
                        ...
                      ]
                  }
                }
              },
              ...
            }
          }

    Args:

        resource_info - a StackInfo object representing the stack with the metadata.  Can be of type ProjectInfo or ResourceGroupInfo

        problems - A ProblemList object used to report problems
    
    Returns:
   
      A dictionary constructed from the resource metadata:
    
          {
             '<resource-arn>': [
                {
                   "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
                   "Action": [ "<allowed-action>", ... ],
                   "ResourceSuffix": [ "<resource-suffix>", ... ],
                   "LogicalResourceId": "<logical-resource-id>"
                },
                ...
             ]
          }

    '''

    resource_definitions = resource_info.resource_definitions
    permissions = {}
    print 'Permission context: ', resource_info.permission_context_name
    for resource in resource_info.resources:

        permission_list = []

        permission_metadata = resource.get_cloud_canvas_metadata('Permissions')
        if permission_metadata is not None:
            print 'Found permission metadata on stack {} resource {}: {}.'.format(resource_info.stack_name, resource.logical_id, permission_metadata)
            problems.push_prefix('Stack {} resource {} ', resource_info.stack_name, resource.logical_id)
            permission_list.extend(_get_permission_list(resource_info.permission_context_name, resource.logical_id, permission_metadata, problems))
            problems.pop_prefix()

        resource_type = resource_definitions.get(resource.type, None)
        permission_metadata = None if resource_type is None else resource_type.permission_metadata
        default_role_mappings = None if permission_metadata is None else permission_metadata.get('DefaultRoleMappings', None)
        if default_role_mappings:
            print 'Found default permission metadata for stack {} resource {} with type {}: {}.'.format(resource_info.stack_name, resource.logical_id, resource.type, permission_metadata)
            problems.push_prefix('Stack {} resource {} default ', resource_info.stack_name, resource.logical_id)
            permission_list.extend(_get_permission_list(resource_info.permission_context_name, resource.logical_id, default_role_mappings, problems))
            problems.pop_prefix()

        if permission_list:
            try:
                # A type to ARN format mapping may not be available.
                resource_arn_type = resource.resource_arn
                existing_list = permissions.get(resource_arn_type, [])
                existing_list.extend(permission_list)
                permissions[resource_arn_type] = existing_list
            except Exception as e:
                problems.append('type {} is not supported by the Custom::AccessControl resource: {}'.format(
                    resource.type,
                    e.message))

        _check_restrictions(resource, permission_metadata, permission_list, problems)

    return permissions


def _check_restrictions(resource, permission_metadata, permission_list, problems):
    restrictions = None if permission_metadata is None else permission_metadata.get('RestrictActions', None)
    if restrictions is None:
        return
    restrictions = set(restrictions)
    for permission in permission_list:
        for action in permission["Action"]:
            if action not in restrictions:
                problems.append("Action {} is not allowed on Resource Type {}".format(action, resource.type))



def _get_permission_list(resource_name, logical_resource_id, permission_metadata_list, problems):
    '''Constructs a list of permissions from resource matadata.

    Args:

        resource_name - the name of the resource or resource group that defines the resource with the permission

        logical_resource_id - the logical id of the resource with the permission

        permission_metadata_list - The permission metadata from the resource. 

            [                                                           # optional list or a single object
                {
                    "AbstractRole": [ "<abstract-role-name>", ... ],    # required list or single string
                    "Action": [ "<allowed-action>", ... ]               # required list or single string
                    "ResourceSuffix": [ "<resource-suffix>", ... ]      # required list or single string
                },
                ...
            ]

        problems - A ProblemList object used to report problems
    
    Retuns:
   
        A list constructed from the provided metadata:

            [
                {
                    "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
                    "Action": [ "<allowed-action>", ... ],
                    "ResourceSuffix": [ "<resource-suffix>", ... ],
                    "LogicalResourceId": "<logical-resource-id>"
                },
                ...
            ]
    '''

    permission_list = []

    if not isinstance(permission_metadata_list, list):
        permission_metadata_list = [ permission_metadata_list ]

    for permission_metadata in permission_metadata_list:
        permission = _get_permission(resource_name, logical_resource_id, permission_metadata, problems)
        if permission is not None:
            permission_list.append(permission)

    return permission_list


def _get_permission(resource_group_name, logical_resource_id, permission_metadata, problems):
    '''Constructs a permission from resource matadata.

    Args:

        resource_group_name - the name of the resource group that defines the resource with the permission

        logical_resource_id - the logical id of the resource with the permission

        permission_metadata - The permission metadata from the resource. 

            {
                "AbstractRole": [ "<abstract-role-name>", ... ],    # required list or single string
                "Action": [ "<allowed-action>", ... ]               # required list or single string
                "ResourceSuffix": [ "<resource-suffix>", ... ]      # optional list or single string
            }

        problems - A ProblemList object used to report problems
    
    Retuns:
   
        A dict constructed from the provided metadata:

            {
                "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
                "Action": [ "<allowed-action>", ... ],
                "ResourceSuffix": [ "<resource-suffix>", ... ],
                "LogicalResourceId": "<logical-resource-id>"
            }

    '''

    # Is a dict?
    if not isinstance(permission_metadata, dict):
        problems.append('CloudCanvas.AccessControl.Permission metadata not an object or list of objects: {}.'.format(json.dumps(permission_metadata)))
        return None

    # Has only allowed properties?
    for property in permission_metadata:
        if property not in ['AbstractRole', 'Action', 'ResourceSuffix']:
            problems.append('CloudCanvas.AccessControl.Permission metadata has unsupported property {}.'.format(property))

    abstract_role_list = _get_permission_abstract_role_list(resource_group_name, permission_metadata.get('AbstractRole'), problems)
    allowed_action_list = _get_permission_allowed_action_list(permission_metadata.get('Action'), problems)
    resource_suffix_list = _get_permission_resource_suffix_list(permission_metadata.get('ResourceSuffix'), problems)

    return {
        'AbstractRole': abstract_role_list,
        'Action': allowed_action_list,
        'ResourceSuffix': resource_suffix_list,
        'LogicalResourceId': logical_resource_id
    }


def _get_permission_abstract_role_list(resource_group_name, abstract_role_list, problems):
    '''Constructs an abstract role list from resource matadata.

    Args:

        resource_group_name - the name of the resource group that defines the resource with the permission

        abstract_role_list - The AbstractRole property from the permission metadata. 

            [ "<abstract-role-name>", ... ],    # required list or single string

        problems - A ProblemList object used to report problems
    
    Retuns:
   
        A list constructed from the provided metadata:

            [ ["<resource-group-name>", "<abstract-role-name>"], ... ],

    '''

    result_list = []

    if abstract_role_list is None:

        problems.append('CloudCanvas.AccessControl.Permission metadata missing required AbstractRole property.')

    else:

        if not isinstance(abstract_role_list, list):
            abstract_role_list = [ abstract_role_list ]

        for abstract_role in abstract_role_list:

            if not isinstance(abstract_role, basestring):
                problems.append('CloudCanvas.AccessControl.Permission metadata property AbstractRole value is not a string or list of strings: {}.'.format(
                    abstract_role))
                continue

            if abstract_role.count('.') != 0:
                problems.append('CloudCanvas.AccessControl.Permission metadata property AbstractRole value {} cannot contain any . characters.'.format(
                    abstract_role))
                continue

            result_list.append( [ resource_group_name, abstract_role ] )

    return result_list


def _get_permission_resource_suffix_list(resource_suffix_list, problems):
    '''Constructs a resource suffix list from resource matadata.

    Args:

        resource_suffix_list - The ResourceSuffix property from the permission metadata. 

            "ResourceSuffix": [ "<resource-suffix>", ... ]      # optional list or single string

        problems - A ProblemList object used to report problems
    
    Retuns:
   
        A list constructed from the provided metadata. The list will have one entry, '', if
        no value is provided in the metadata.

            [ "<resource-suffix>", ... ],

    '''

    result_list = []

    if resource_suffix_list is None or (isinstance(resource_suffix_list, list) and len(resource_suffix_list) == 0):

        result_list.append('') # _update_role needs at least one entry in the list

    else:

        if not isinstance(resource_suffix_list, list):
            resource_suffix_list = [ resource_suffix_list ]

        for resource_suffix in resource_suffix_list:

            if not isinstance(resource_suffix, basestring):
                problems.append('CloudCanvas.AccessControl.Permission metadata property ResourceSuffix value is not a string or list of strings: {}.'.format(
                    resource_suffix))
                continue

            result_list.append(resource_suffix)

    return result_list


def _get_permission_allowed_action_list(allowed_action_list, problems):
    '''Constructs a allowed action list from resource matadata.

    Args:

        allowed_action_list - The Action property value from the metadata 

            "Action": [ "<allowed-action>", ... ]               # required list or single string

        problems - A ProblemList object used to report problems
    
    Retuns:
   
        A list constructed from the provided metadata:

            "Action": [ "<allowed-action>", ... ],

    '''

    result_list = []

    if allowed_action_list is None:

        problems.append('CloudCanvas.AccessControl.Permission metadata missing required Action property.')

    else:

        if not isinstance(allowed_action_list, list):
            allowed_action_list = [ allowed_action_list ]

        for allowed_action in allowed_action_list:

            if not isinstance(allowed_action, basestring):
                problems.append('CloudCanvas.AccessControl.Permission metadata property Action value is not a string or list of strings: {}.'.format(
                    allowed_action))
                continue

            result_list.append(allowed_action)

    return result_list


def _get_implicit_role_mappings(resource_info, problems):

    '''Looks in a resource group stack for roles created by custom resources such as Custom::LambdaConfiguration
    and Custom::ServiceApi. 
    
    Args:

        resource_info - a StackInfo object representing the stack with the metadata.  Can be of type ProjectInfo or ResourceGroupInfo
    
        problems - A ProblemList object used to report problems
    
    Returns:

        A dictionary containing the following data for each role discovered:
    
          {
            "<role-physical-resource-id>": [
              {
                "Effect": "<allow-or-deny>",
                "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
              },
              ...
            ]
          }

    '''

    role_mappings = {}
    for resource in resource_info.resources:
        if resource.type.startswith('Custom::'):
            id_data = aws_utils.get_data_from_custom_physical_resource_id(resource.physical_id)
            resource_role_mappings = role_utils.get_id_data_abstract_role_mappings(id_data)
            for abstract_role_name, physical_role_name in resource_role_mappings.iteritems():
                print 'Adding implicit abstract role {} mapping to physical role {} for stack {}.'.format(abstract_role_name, physical_role_name, resource_info.stack_name)
                role_mapping_list = role_mappings.setdefault(
                    physical_role_name, 
                    [
                        {
                            'Effect': 'Allow',
                            'AbstractRole': []
                        }                    
                    ]
                )
                role_mapping_list[0]['AbstractRole'].append(                                
                    [
                        resource_info.permission_context_name,
                        abstract_role_name
                    ] 
                )

    return role_mappings

def _get_explicit_role_mappings(stack, problems):
    '''Looks for the folling metadata in a stack template:
    
          {
            "Resources": {
              "<role-logical-resource-id>": {
                "Metadata": {                                                                     # optional
                  "CloudCanvas": {                                                                # optional
                    "AccessControl": {                                                            # optional
                      "RoleMappings": [                                                           # optional list or a single object
                        {
                          "AbstractRole": [ "<resource-group-name>.<abstract-role-name>", ... ],  # required list or single string
                          "Effect": "<allow-or-deny>",                                            # required "Allow" or "Deny"
                        },
                        ...
                      ]
                    }
                  }
                }
              },
              ...
            }
          }     

    Args:

        stack - a stack_info.StackInfo object representing the stack with the matadata
    
        problems - A ProblemList object used to report problems
    
    Returns:

        A dictionary constructed from that data:
    
          {
            "<role-physical-resource-id>": [
              {
                "Effect": "<allow-or-deny>",
                "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
              },
              ...
            ]
          }

    '''

    role_mappings = {}

    # Get all the role resources defined in the target stack.

    role_resources = stack.resources.get_by_type('AWS::IAM::Role')
    for role_resource in role_resources:

        problems.push_prefix('Stack {} resource {} ', stack.stack_name, role_resource.logical_id)

        role_mapping_list = []

        role_mapping_metadata_list = role_resource.get_cloud_canvas_metadata('RoleMappings')
        if role_mapping_metadata_list is not None:
            print 'Found role mapping metadata on stack {} resource {}: {}'.format(stack.stack_name, role_resource.logical_id, role_mapping_metadata_list)
            role_mapping_list = _get_role_mapping_list(role_mapping_metadata_list, problems)

        role_mappings[role_resource.physical_id] = role_mapping_list

        problems.pop_prefix()

    return role_mappings


def _get_role_mapping_list(role_mapping_metadata_list, problems):
    '''Constructs a role mapping list from resource metadata.
    
    Args:

        role_mapping_metadata_list - The resource metadata.

            [                                                                               # optional list or a single object
                {
                    "AbstractRole": [ "<resource-group-name>.<abstract-role-name>", ... ],  # required list or single string
                    "Effect": "<allow-or-deny>",                                            # required "Allow" or "Deny"
                },
                ...
            ]
    
        problems - A ProblemList object used to report problems
    
    Returns:

        A list constructed from that data:
    
            [
              {
                "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
                "Effect": "<allow-or-deny>",
              },
              ...
            ]

    '''

    role_mapping_list = []

    if not isinstance(role_mapping_metadata_list, list):
        role_mapping_metadata_list = [ role_mapping_metadata_list ]

    for role_mapping_metadata in role_mapping_metadata_list:
        role_mapping = _get_role_mapping(role_mapping_metadata, problems)
        if role_mapping is not None:
            role_mapping_list.append(role_mapping)

    return role_mapping_list


def _get_role_mapping(role_mapping_metadata, problems):
    '''Constructs a role mapping from resource metadata.
    
    Args:

        role_mapping_metadata - The resource metadata.

            {
                "AbstractRole": [ "<resource-group-name>.<abstract-role-name>", ... ],  # required list or single string
                "Effect": "<allow-or-deny>",                                            # required "Allow" or "Deny"
            }
    
        problems - A ProblemList object used to report problems
    
    Returns:

        A dict constructed from the metadata:
    
            {
                "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
                "Effect": "<allow-or-deny>",
            }

    '''

    # Validate the mapping is a dict with the required properties 
    # and only supported properties.

    if not isinstance(role_mapping_metadata, dict):
        problems.append('CloudCanvas.AccessControl.RoleMappings metadata not an object or list of objects: {}.'.format(
            json.dumps(role_mapping_metadata)))
        return None

    for property in role_mapping_metadata:
        if property not in ['AbstractRole', 'Effect']:
            problems.append('CloudCanvas.AccessControl.RoleMappings metadata has unsupported property {}.'.format(
                property))

    effect = _get_role_mapping_effect(role_mapping_metadata.get('Effect'), problems)
    abstract_role_list = _get_role_mapping_abstract_role_list(role_mapping_metadata.get('AbstractRole'), problems)

    return {
        'Effect': effect,
        'AbstractRole': abstract_role_list
    }


def _get_role_mapping_effect(effect, problems):
    '''Constructs a role mapping effect from resource metadata.
    
    Args:

        effect - The value of the 'Effect' property in the resource metadata.

            "<allow-or-deny>" # required "Allow" or "Deny"
    
        problems - A ProblemList object used to report problems
    
    Returns:

        Either 'Allow' or 'Deny'.
    
    '''

    if effect is None:
        problems.append('CloudCanvas.AccessControl.RoleMappings metadata missing required Effect property.')
    else:
        if effect not in ['Allow', 'Deny']:
            problems.append('CloudCanvas.AccessControl.RoleMappings metadata property Effect value is not Allow or Deny.')

    return effect


def _get_role_mapping_abstract_role_list(abstract_role_list, problems):
    '''Constructs an abstract role list from resource metadata.
    
    Args:

        abstract_role_list - The value of the "AbstractRole" property from the resource metadata.

            "AbstractRole": [ "<resource-group-name>.<abstract-role-name>", ... ],  # required list or single string
    
        problems - A ProblemList object used to report problems
    
    Returns:

        A list constructed from the metadata:
    
            [ ["<resource-group-name>", "<abstract-role-name>"], ... ]

    '''

    result_list = []

    if abstract_role_list is None:

        problems.append('CloudCanvas.AccessControl.RoleMappings metadata missing required AbstractRole property.')

    else:

        if not isinstance(abstract_role_list, list):
            abstract_role_list = [ abstract_role_list ]

        for abstract_role in abstract_role_list:

            if not isinstance(abstract_role, basestring):
                problems.append('CloudCanvas.AccessControl.Permission metadata property AbstractRole value is not a string or list of strings: {}.'.format(
                    abstract_role))
                continue

            if abstract_role.count('.') != 1:
                problems.append('CloudCanvas.AccessControl.Permission metadata property AbstractRole value {} should have the form "<resource-group-name>.<abstract-role-name>" where <resource-group-name> can be "*".'.format(
                    abstract_role))
                continue

            result_list.append(abstract_role.split('.'))

    return result_list


def _update_roles(request_type, policy_name, permissions, role_mappings):
    '''Update roles to reflect the desired permissions.

    Args:

        request_type - 'Create', 'Update', or 'Delete'. Delete causes the named policy to be deleted.
        Create and Update causes the named policy to be set.

        policy_name - The name of the policy to delete or set.

        permissions - A description of the desired permissions:

          {
             '<resource-arn>': [
                {
                   "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
                   "Action": [ "<allowed-action>", ... ],
                   "ResourceSuffix": [ "<resource-suffix>", ... ],
                   "LogicalResourceId": "<logical-resource-id>"
                },
                ...
             ]
          }

        role_mappings - A abstract to physical role mappings that identify the roles to be updated:

          {
            "<role-physical-resource-id>": [
              {
                "Effect": "<allow-or-deny>",
                "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
              },
              ...
            ]
          }

    '''
    
    were_changes = False

    print 'Updating roles for {} request using permissions {} and role_mappings {}.'.format(request_type, permissions, role_mappings)

    if request_type == 'Delete':

        # Delete the target policy from all of roles. It is OK if the policy
        # doesn't exist.

        for role_physical_id in role_mappings:
            try:
                print 'Delete requested. Deleting role policy {} from role {}.'.format(policy_name, role_physical_id)
                iam.delete_role_policy(RoleName=role_physical_id, PolicyName=policy_name)
                were_changes = True
            except ClientError as e:
                if e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
                    raise e    

    else:

        for role_physical_id, role_mapping_list in role_mappings.iteritems():

            # Generate a policy for the role. If the policy has no statements,
            # we delete the policy instead of putting it. In this case, it is ok
            # if the policy doesn't exist.

            policy = _create_role_policy(permissions, role_mapping_list)
            if len(policy['Statement']) == 0:
                try:
                    print '{} requested. Deleting role policy {} from role {} because there are no statements in the policy.'.format(request_type, policy_name, role_physical_id)
                    iam.delete_role_policy(RoleName=role_physical_id, PolicyName=policy_name)
                    were_changes = True
                except ClientError as e:
                    if e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
                        raise e    
            else:
                policy_content = json.dumps(policy)
                print '{} requested. Putting role policy {} on role {}: {}'.format(request_type, policy_name, role_physical_id, policy_content)
                iam.put_role_policy(RoleName=role_physical_id, PolicyName=policy_name, PolicyDocument=policy_content)
                were_changes = True

    return were_changes

def _create_role_policy(permissions, role_mapping_list):
    '''Create a policy document that implements a resource groups permissions for a role.

    Args:

        permissions - a dictionary of the form:

            {
               '<resource-arn>': [
                  {
                     "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
                     "Action": [ "<allowed-action>", ... ],
                     "ResourceSuffix": [ "<resource-suffix>", ... ],
                     "LogicalResourceId": "<logical-resource-id>"
                  },
                  ...
               ]
            }

        role_mapping_list - a list of the form:

            [
              {
                "Effect": "<allow-or-deny>",
                "AbstractRole": [ ["<resource-group-name>", "<abstract-role-name>"], ... ],
              },
              ...
            ]

    Returns:

        A dict that is a policy document with statements constructed from the permissions and 
        role_mapping_list data where the respective resource-group-name and abstract-role-names
        match.

    '''

    print 'Creating role policy for permissions {} and role_mapping_list {}.'.format(json.dumps(permissions), json.dumps(role_mapping_list))

    sid_counts = {}

    statements = []
    for resource_arn, permission_list in permissions.iteritems():
        for permission in permission_list:
            for role_mapping in role_mapping_list:

                # If the role mapping is for the abstract role specified by the
                # permission, then add an AccessControl statement to the policy.

                if _any_abstract_roles_match(permission.get('AbstractRole'), role_mapping.get('AbstractRole')):

                    sid = permission['LogicalResourceId']
                    if sid in sid_counts:
                        sid_count = sid_counts[sid] + 1
                        sid_counts[sid] = sid_count
                    else:
                        sid_count = 1
                        sid_counts[sid] = sid_count

                    sid = sid + str(sid_count)

                    statement = {
                        'Sid': sid,
                        'Effect': role_mapping['Effect'],
                        'Action': permission['Action'],
                        'Resource': [ resource_arn + resource_suffix for resource_suffix in permission['ResourceSuffix'] ]
                    }
                    statements.append(statement)

                    print 'Added statement to policy: {}'.format(statement)


    # Return the policy.

    policy = {
        'Version': '2012-10-17',
        'Statement': statements
    }

    print 'Created policy: {}'.format(policy)

    return policy


def _any_abstract_roles_match(permission_abstract_role_list, mapping_abstract_role_list):
    '''Determins if a permission abstract role matches a mapping abstract role.
    
    Args:
    
        Both input values have the form:
    
            [ ("<resource-group-name>", "<abstract-role-name>"), ... ]
    
    Returns:
        
        True if there exists a tuple from both lists where both parts are the same or if the
        <resource-group-name> part of the mapping is '*' and the <abstract-role-name> parts 
        match.

    '''
    for permission_abstract_role in permission_abstract_role_list:
        for mapping_abstract_role in mapping_abstract_role_list:

            if permission_abstract_role == mapping_abstract_role:
                print 'Exact abstract role match for {} and {}.'.format(permission_abstract_role, mapping_abstract_role)
                return True

            if mapping_abstract_role[0] == '*' and permission_abstract_role[1] == mapping_abstract_role[1]:
                print 'Wildcard abstract role match for {} and {}.'.format(permission_abstract_role, mapping_abstract_role)
                return True

    print 'No role match for {} and {}.'.format(permission_abstract_role_list, mapping_abstract_role_list)
    return False



    