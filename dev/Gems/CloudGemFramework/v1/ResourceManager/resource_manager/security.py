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
# $Revision: #9 $
import json
import collections
import six
import time

from .errors import HandledError
from cgf_utils import aws_iam_role
from cgf_utils import custom_resource_utils


DEFAULT_PATCH_IDENTIFIER = '032020'    # Identifies current default patch to apply

def add_cli_commands(subparsers, add_common_args):
    __add_role_cli_commands(subparsers, add_common_args)
    __add_role_mapping_cli_commands(subparsers, add_common_args)
    __add_permission_cli_commands(subparsers, add_common_args)

    __add_project_patcher(subparsers, add_common_args)


def __add_role_cli_commands(subparsers, add_common_args):
    parser = subparsers.add_parser('role',
                                   help='Add, modify, and remove AWS IAM Role definitions in the project-template.json and deployment-access-template.json files.')
    subparsers = parser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    # role add [--project] --role ROLE-NAME
    subparser = subparsers.add_parser('add', help='Adds an AWS IAM Role definition to the project-template.json or deployment-access-template.json files.')
    subparser.add_argument('--project', action='store_true',
                           help='Indicates that the role definition should go in the project-template.json file. The default is to put it in the deployment-access-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ROLE-NAME', help='The name of the role definition.')
    add_common_args(subparser)
    subparser.set_defaults(func=add_role)

    # role remove [--project] --role ROLE-NAME
    subparser = subparsers.add_parser('remove',
                                      help='Removes an AWS IAM Role definition from the project-template.json or deployment-access-template.json files.')
    subparser.add_argument('--project', action='store_true',
                           help='Indicates that the role definition should be removed from the project-template.json file. The default is to remove it from the deployment-access-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ROLE-NAME', help='The name of the role definition.')
    add_common_args(subparser)
    subparser.set_defaults(func=remove_role)

    # role list [--project|--deployment]
    subparser = subparsers.add_parser('list', help='Lists the AWS IAM Role definitions in the project-template.json and deployment-access-template.json files.')
    group = subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('--project', action='store_true',
                       help='Lists role definitions in the project-template.json file. The default is to list definitions in the project-template.json and deployment-access-template.json files.')
    group.add_argument('--deployment', action='store_true',
                       help='Lists role definitions in the deployment-access-template.json file. The default is to list role definitions in the project-template.json and deployment-access-template.json files.')
    add_common_args(subparser)
    subparser.set_defaults(func=list_roles)


def __add_role_mapping_cli_commands(subparsers, add_common_args):
    parser = subparsers.add_parser('role-mapping',
                                   help='Add, modify, and remove Cloud Canvas RoleMappings metadata on AWS IAM Role definitions in the project-template.json and deployment-access-template.json files.')
    subparsers = parser.add_subparsers(dest='subparser_name')

    # role-mapping add [--project] --role ROLE-NAME --pattern ABSTRACT-ROLE-PATTERN --allow|--deny
    subparser = subparsers.add_parser('add',
                                      help='Adds Cloud Canvas RoleMappings metadata to an AWS IAM Role definition in the project-template.json or deployment-access-template.json file.')
    subparser.add_argument('--project', action='store_true',
                           help='Indicates that the role definition is in the project-template.json file. The default is for the role definition to be in the deployment-access-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ROLE-NAME', help='The name of the role definition.')
    subparser.add_argument('--pattern', metavar='ABSTRACT-ROLE-PATTERN', required=True,
                           help='Identifies the abstract roles mapped to the role. Has the form <resource-group-name>.<abstract-role-name>, where <resource-group-name> can be *.')
    group = subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('--allow', action='store_true', help='Indicates that the permissions requested for the abstract role are allowed.')
    group.add_argument('--deny', action='store_true', help='Indicates that the permissions requested for the abstract role are denied.')
    add_common_args(subparser)
    subparser.set_defaults(func=add_role_mapping)

    # role-mapping remove [--project] --role ROLE-NAME --pattern ABSTRACT-ROLE-PATTERN
    subparser = subparsers.add_parser('remove',
                                      help='Removes Cloud Canvas RoleMappings metadata from an AWS IAM Role definition in the project-template.json or deployment-access-template.json file.')
    subparser.add_argument('--project', action='store_true',
                           help='Indicates that the role definition is in the project-template.json file. The default is for the role definition to be in the deployment-access-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ROLE-NAME', help='The name of the role definition.')
    subparser.add_argument('--pattern', required=True, metavar='ABSTRACT-ROLE-PATTERN',
                           help='Identifies the abstract roles mapped to the role. Has the form <resource-group-name>.<abstract-role-name>, where <resource-group-name> can be *.')
    add_common_args(subparser)
    subparser.set_defaults(func=remove_role_mapping)

    # role-mapping list [--project|--deployment] [--role ROLE-NAME] [--pattern ABSTRACT-ROLE-PATTERN]
    subparser = subparsers.add_parser('list',
                                      help='Lists Cloud Canvas RoleMappings metadata on the AWS IAM Role definitions in the project-template.json and deployment-access-template.json files.')
    group = subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('--project', action='store_true',
                       help='Lists metadata from role definitions in the project-template.json file. The default is to list metadata from role definitions in the project-template.json and deployment-access-template.json files.')
    group.add_argument('--deployment', action='store_true',
                       help='Lists metadata from role definitions in the deployment-access-template.json file. The default is to list metadata from role definitions in the project-template.json and deployment-access-template.json files.')
    subparser.add_argument('--role', metavar='ROLE-NAME', required=False,
                           help='The role definition with the metadata to list. The default is to list metadata from all role definitions.')
    subparser.add_argument('--pattern', metavar='ABSTRACT-ROLE-PATTERN',
                           help='The abstract role pattern specified by the metadata listed. The default is to list metadata with any abstract role pattern.')
    add_common_args(subparser)
    subparser.set_defaults(func=list_role_mappings)


def __add_permission_cli_commands(subparsers, add_common_args):
    parser = subparsers.add_parser('permission',
                                   help='Add, modify, and remove Cloud Canvas Permissions mapping metadata on resource definitions in resource-group-template.json files.')
    subparsers = parser.add_subparsers(dest='subparser_name')

    # permission add --resource-group RESOURCE-GROUP-NAME --resource RESOURCE-NAME --role ABSTRACT-ROLE-NAME --action ACTION ... [--suffix SUFFIX ...]
    subparser = subparsers.add_parser('add', help='Adds Cloud Canvas Permissions metadata to an resource definition in a resource-group-template.json file.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP-NAME', required=True,
                           help='The name of a resource group. The metadata will be added to a resource definition in that resource group\'s resource-group-template.json file.')
    subparser.add_argument('--resource', metavar='RESOURCE-NAME', required=True,
                           help='The name of the resource definition in the resource-group-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ABSTRACT-ROLE-NAME', help='Identifies the role that will be granted the permission.')
    subparser.add_argument('--action', required=False, nargs='+', metavar='ACTION', help='The action that is allowed. May be specified more than once.')
    subparser.add_argument('--suffix', required=False, nargs='+', metavar='SUFFIX',
                           help='A string appended to the resource ARN. May be specified more than once.')
    add_common_args(subparser)
    subparser.set_defaults(func=add_permission)

    # permission remove --resource-group RESOURCE-GROUP-NAME --resource RESOURCE-NAME --role ABSTRACT-ROLE-NAME ... [--action ACTION ...] [--suffix SUFFIX ...]
    subparser = subparsers.add_parser('remove',
                                      help='Removes Cloud Canvas Permissions metadata from a resource definition in a resource-group-template.json file.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP-NAME', required=True,
                           help='The name of a resource group. The metadata will be removed from a resource definition in that resource group\'s resource-group-template.json file.')
    subparser.add_argument('--resource', metavar='RESOURCE-NAME', required=True,
                           help='The name of the resource definition in the resource-group-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ABSTRACT-ROLE-NAME', help='Identifies the roles from which permissions are removed.')
    subparser.add_argument('--action', required=False, nargs='+', metavar='ACTION', help='The action that is removed. May be specified more than once.')
    subparser.add_argument('--suffix', required=False, nargs='+', metavar='SUFFIX',
                           help='A string appended to the resourceARN, which is removed. May be specified more than once.')
    add_common_args(subparser)
    subparser.set_defaults(func=remove_permission)

    # permission list [--resource-group RESOURCE-GROUP-NAME] [--resource RESOURCE-NAME] [--role ABSTRACT-ROLE-NAME]
    subparser = subparsers.add_parser('list',
                                      help='Removes Cloud Canvas Permissions metadata from a resource definition in a resource-group-template.json file.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP-NAME',
                           help='Will list the metadata from resource definitions in the resource group\'s resource-group-template.json file. The default is to list permissions from all resource groups.')
    subparser.add_argument('--resource', metavar='RESOURCE-NAME',
                           help='The name of the resource definition in the resource-group-template.json file. The default is to list metadata from all resource definitions.')
    subparser.add_argument('--role', metavar='ABSTRACT-ROLE-NAME',
                           help='Lists metadata for the specified abstract role. The default is to list metadata for all abstract roles.')
    add_common_args(subparser)
    subparser.set_defaults(func=list_permissions)


def __add_project_patcher(subparsers, add_common_args):
    parser = subparsers.add_parser('patcher',
                                   help='Patch deployed project resources based on best security practices. Recommend running backup before patching project and deployment')
    subparsers = parser.add_subparsers(dest='subparser_name')

    # security patch
    subparser = subparsers.add_parser('patch', help='Patch deployed resources following best security practices.')
    subparser.add_argument('--deployment', metavar='DEPLOYMENT', required=False,
                           help='The name of the deployment to update. If deployment name is none, defaults to patching project stack')
    subparser.add_argument('--silent-patch', default=False, required=False, action='store_true', help='Run the command silently without output.')
    subparser.add_argument('--dryrun', default=False, required=False, action='store_true', help='Run the command without modifying stacks.')
    subparser.add_argument('--identifier', required=False, metavar='IDENTIFIER',
                           help='Identifies which patch identifier to apply. Defaults to 0302019.')
    add_common_args(subparser)
    subparser.set_defaults(func=run_project_patcher)


DEFAULT_ACCESS_CONTROL_RESOURCE_DEFINITION = {
    "Type": "Custom::AccessControl",
    "Properties": {
        "ServiceToken": {"Ref": "ProjectResourceHandler"},
        "ConfigurationBucket": {"Ref": "ConfigurationBucket"},
        "ConfigurationKey": {"Ref": "ConfigurationKey"}
    },
    "DependsOn": []
}


def ensure_access_control(template, resource):
    """Make sure an AccessControl resource exists and that it depends on a resource.

    Args:

        template - the template to check

        resource - the name of the resource

    Returns:

        True if the resource was added, False if it was already there.
    """

    changed = False

    resources = template.setdefault('Resources', {})

    access_control = resources.setdefault('AccessControl', DEFAULT_ACCESS_CONTROL_RESOURCE_DEFINITION)

    dependencies = access_control.setdefault('DependsOn', [])
    if not isinstance(dependencies, list):
        dependencies = [dependencies]
        access_control['DependsOn'] = dependencies

    if resource not in dependencies:
        dependencies.append(resource)
        changed = True

    return changed


def ensure_no_access_control(template, resource):
    """Make sure the AccessControl resource, if it exists, does not depend on a resource.

    Args:

        template - the template to check

        resource - the name of the resource

    Returns:

        True if the resource was removed, False if it wasn't there.
    """
    changed = False
    resources = template.get('Resources', {})

    access_control = resources.get('AccessControl', {})

    dependencies = access_control.get('DependsOn', [])
    if isinstance(dependencies, list) and resource in dependencies:
        dependencies.remove(resource)
        changed = True
    elif isinstance(dependencies, six.string_types) and resource == dependencies:
        access_control['DependsOn'] = []
        changed = True

    return changed


TargetTemplate = collections.namedtuple('TargetTemplate', 'template file_path save role_path scope')


def __get_target_template(context, args):
    if args.project:
        return TargetTemplate(
            template=context.config.project_template_aggregator.extension_template,
            file_path=context.config.project_template_aggregator.extension_file_path,
            save=context.config.project_template_aggregator.save_extension_template,
            role_path={"Fn::Join": ["", ["/", {"Ref": "AWS::StackName"}, "/"]]},
            scope='project')
    else:
        return TargetTemplate(
            template=context.config.deployment_template_aggregator.extension_template,
            file_path=context.config.deployment_template_aggregator.extension_file_path,
            save=context.config.deployment_template_aggregator.save_extension_template,
            role_path={"Fn::Join": ["", ["/", {"Ref": "ProjectStack"}, "/", {"Ref": "DeploymentName"}, "/"]]},
            scope='deployment')


# role add [--project] --role ROLE-NAME
def add_role(context, args):
    target = __get_target_template(context, args)

    resources = target.template.setdefault('Resources', {})

    if args.role in resources:
        raise HandledError('A resource with the name {} already exists in the {} template.'.format(args.role, target.file_path))

    resources[args.role] = {
        "Type": "AWS::IAM::Role",
        "Properties": {
            "Path": target.role_path,
            "AssumeRolePolicyDocument": {
                "Version": "2012-10-17",
                "Statement": [
                    {
                        "Sid": "AccountUserAssumeRole",
                        "Effect": "Allow",
                        "Action": "sts:AssumeRole",
                        "Principal": {"AWS": {"Ref": "AWS::AccountId"}}
                    }
                ]
            }
        },
        "Metadata": {
            "CloudCanvas": {
                "RoleMappings": []
            }
        }
    }

    ensure_access_control(target.template, args.role)

    target.save()

    context.view.role_added(target.scope, args.role)


# role remove [--project] --role ROLE-NAME
def remove_role(context, args):
    target = __get_target_template(context, args)

    resources = target.template.setdefault('Resources', {})

    if args.role not in resources:
        raise HandledError('No {} resource found in the {} template.'.format(args.role, target.file_path))

    if resources[args.role].get('Type') != 'AWS::IAM::Role':
        raise HandledError('Resource {} in the {} template has type {} not type AWS::IAM::Role.'.format(args.role, target.file_path,
                                                                                                        resources[args.role].get('Type', '(none)')))

    if args.role in ['Player', 'DeploymentAdmin', 'DeploymentOwner', 'ProjectAdmin', 'ProjectOwner', 'ProjectResourceHandlerExecution',
                     'PlayerAccessTokenExchangeExecution']:
        raise HandledError('The {} role is required by Cloud Canvas and cannot be removed.'.format(args.role))

    del resources[args.role]

    ensure_no_access_control(target.template, args.role)

    target.save()

    context.view.role_removed(target.scope, args.role)


# role list [--project|--deployment]
def list_roles(context, args):
    role_list = []

    if not args.deployment:
        role_list.extend(__list_roles_in_template(context.config.project_template_aggregator.effective_template, 'Project'))

    if not args.project:
        role_list.extend(__list_roles_in_template(context.config.deployment_access_template_aggregator.effective_template, 'Deployment'))

    context.view.role_list(role_list)


def __list_roles_in_template(template, scope):
    role_list = []

    for resource, definition in six.iteritems(template.get('Resources', {})):
        if definition.get('Type') == 'AWS::IAM::Role':
            role_list.append(
                {
                    'Name': resource,
                    'Scope': scope
                }
            )

    return role_list


# role-mapping add [--project] --role ROLE-NAME --pattern ABSTRACT-ROLE-PATTERN --allow|--deny
def add_role_mapping(context, args):
    target = __get_target_template(context, args)

    resources = target.template.get('Resources', {})

    role_resource = resources.get(args.role)

    if not role_resource:
        raise HandledError('No {} resource found in the {} template.'.format(args.role, target.file_path))

    if role_resource.get('Type') != 'AWS::IAM::Role':
        raise HandledError(
            'Resource {} in the {} template has type {} not type AWS::IAM::Role.'.format(args.role, target.file_path, role_resource.get('Type', '(none)')))

    _validate_abstract_role_pattern(args.pattern)

    effect = 'Allow' if args.allow else 'Deny'

    role_mappings_metadata = role_resource.setdefault('Metadata', {}).setdefault('CloudCanvas', {}).setdefault('RoleMappings', [])

    if not isinstance(role_mappings_metadata, list):
        role_mappings_metadata = [role_mappings_metadata]
        role_resource['Metadata']['CloudCanvas']['RoleMappings'] = role_mappings_metadata

    for existing_role_mapping in role_mappings_metadata:
        if existing_role_mapping.get('AbstractRole') == args.pattern:
            raise HandledError('A role mapping for {} already exists on the {} role definition in {}.'.format(args.pattern, args.role, target.file_path))

    role_mappings_metadata.append(
        {
            'AbstractRole': args.pattern,
            'Effect': effect
        }
    )

    target.save()

    context.view.role_mapping_added(target.scope, args.role, args.pattern)


# role-mapping remove [--project] --role ROLE-NAME --pattern ABSTRACT-ROLE-PATTERN
def remove_role_mapping(context, args):
    target = __get_target_template(context, args)

    resources = target.template.get('Resources', {})

    role_resource = resources.get(args.role)

    if not role_resource:
        raise HandledError('No {} resource found in the {} template.'.format(args.role, target.file_path))

    if role_resource.get('Type') != 'AWS::IAM::Role':
        raise HandledError(
            'Resource {} in the {} template has type {} not type AWS::IAM::Role.'.format(args.role, target.file_path, role_resource.get('Type', '(none)')))

    _validate_abstract_role_pattern(args.pattern)

    role_mappings_metadata = role_resource.setdefault('Metadata', {}).setdefault('CloudCanvas', {}).setdefault('RoleMappings', [])

    if not isinstance(role_mappings_metadata, list):
        role_mappings_metadata = [role_mappings_metadata]
        role_resource['Metadata']['CloudCanvas']['RoleMappings'] = role_mappings_metadata

    new_role_mappings_metadata = [entry for entry in role_mappings_metadata if entry.get('AbstractRole') != args.pattern]

    if len(new_role_mappings_metadata) == len(role_mappings_metadata):
        raise HandledError('No role mapping for {} was found on the role {} in {}.'.format(args.pattern, args.role, target.file_path))

    role_resource['Metadata']['CloudCanvas']['RoleMappings'] = new_role_mappings_metadata

    target.save()

    context.view.role_mapping_removed(target.scope, args.role, args.pattern)


# role-mapping list [--project|--deployment] [--role ROLE-NAME] [--pattern ABSTRACT-ROLE-PATTERN]
def list_role_mappings(context, args):
    role_mapping_list = []

    if not args.deployment:
        role_mapping_list.extend(__list_role_mappings_in_template(context.config.project_template_aggregator.effective_template, 'Project'))

    if not args.project:
        role_mapping_list.extend(__list_role_mappings_in_template(context.config.deployment_access_template_aggregator.effective_template, 'Deployment'))

    context.view.role_mapping_list(role_mapping_list)


def __list_role_mappings_in_template(template, scope):
    result = []
    for resource_name, resource_definition in six.iteritems(template.get('Resources', {})):
        if resource_definition.get('Type') == 'AWS::IAM::Role':
            role_mappings = resource_definition.get('Metadata', {}).get('CloudCanvas', {}).get('RoleMappings', [])
            if not isinstance(role_mappings, list):
                role_mappings = [role_mappings]
            for role_mapping in role_mappings:
                patterns = role_mapping.get('AbstractRole', [])
                if not isinstance(patterns, list):
                    patterns = [patterns]
                for pattern in patterns:
                    result.append(
                        {
                            'Role': resource_name,
                            'Scope': scope,
                            'Pattern': pattern,
                            'Effect': role_mapping.get('Effect', '(none)')
                        }
                    )
    return result


def _validate_abstract_role_pattern(pattern):
    parts = pattern.split('.')
    if len(parts) != 2:
        raise HandledError(
            'The abstract role pattern {} should have the format <resource-group-name>.role-name>, where <resource-group-name> can be *.'.format(pattern))


# permission add --resource-group RESOURCE-GROUP-NAME --resource RESOURCE-NAME --role ABSTRACT-ROLE-NAME [--action ACTION ...] [--suffix SUFFIX ...]
def add_permission(context, args):
    add_permission_to_role(context, args.resource_group, args.resource, args.role, args.action, args.suffix)


def add_permission_to_role(context, target_resource_group, target_resource, role_name, actions=None, suffixes=None):
    resource_group = context.resource_groups.get(target_resource_group)
    resources = resource_group.template.get('Resources', {})

    resource = resources.get(target_resource)
    if not resource:
        raise HandledError('The {} resource group defines no {} resource.'.format(target_resource_group, target_resource))

    permissions = resource.setdefault('Metadata', {}).setdefault('CloudCanvas', {}).setdefault('Permissions', [])
    if not isinstance(permissions, list):
        permissions = [permissions]
        resource['Metadata']['CloudCanvas']['Permissions'] = permissions

    # Look for an existing entry for the abstract role. If there is more
    # than one, then the cli can't be used to edit them.

    permission_for_role = None
    for permission in permissions:

        abstract_roles = permission.setdefault('AbstractRole', [])
        if not isinstance(abstract_roles, list):
            abstract_roles = [abstract_roles]
            permission['AbstractRole'] = abstract_roles

        if role_name in abstract_roles:

            if permission_for_role:
                raise HandledError(
                    'The {} resource {} has more than one permissions metadata object for role {}. The entry cannot be modified using the command line tool. "\
                    "Please edit the definition in the {} template file instead'.format(
                        target_resource_group,
                        target_resource,
                        role_name,
                        resource_group.template_path
                    )
                )

            permission_for_role = permission

    if permission_for_role:

        # There is an existing entry for the abstract role...

        # If the entry is for other roles as well, they can't modify it using the cli.
        abstract_roles = permission_for_role['AbstractRole']
        if isinstance(abstract_roles, list) and len(abstract_roles) > 1:
            raise HandledError(
                'The {} resource {} permissions for role {} contains permissions for more than one role ({}). "\
                "The entry cannot be modified using the command line tool. Please edit the definition in the {} template file instead'.format(
                    target_resource_group,
                    target_resource,
                    role_name,
                    ', '.join(abstract_roles),
                    resource_group.template_path
                )
            )

        # If an action was given, add it to the list of actions.
        if actions:

            existing_actions = permission_for_role.setdefault('Action', [])
            if not isinstance(existing_actions, list):
                existing_actions = [existing_actions]
                permission_for_role['Action'] = existing_actions

            for action in actions:
                if action in existing_actions:
                    raise HandledError(
                        'The {} resource {} permissions for role {} already specify action {}.'.format(target_resource_group, target_resource, role_name,
                                                                                                       action))
                existing_actions.append(action)

        # If a suffix was given, add it to the list of suffixes.
        if suffixes:

            existing_suffixes = permission.setdefault('ResourceSuffix', [])
            if not isinstance(existing_suffixes, list):
                existing_suffixes = [existing_suffixes]
                permissions['ResourceSuffix'] = existing_suffixes

            for suffix in suffixes:
                if suffix in existing_suffixes:
                    raise HandledError(
                        'The {} resource {} permissions for role {} already specify suffix {}.'.format(
                            target_resource_group, target_resource, role_name, suffix))
                existing_suffixes.append(suffix)

    else:
        # There is no existing entry for the abstract role...

        # An action must be specified.
        if not actions:
            raise HandledError(
                'No {} resource {} permissions for role {} are defined. The --action option must be specified.'.format(target_resource_group, target_resource,
                                                                                                                       role_name))

        permission = {
            'AbstractRole': role_name,
            'Action': actions
        }

        if suffixes:
            permission['ResourceSuffix'] = suffixes

        permissions.append(permission)

    resource_group.save_template()

    context.view.permission_added(target_resource_group, target_resource, role_name)


# permission remove --resource-group RESOURCE-GROUP-NAME --resource RESOURCE-NAME --role ABSTRACT-ROLE-NAME [--action ACTION ...] [--suffix SUFFIX ...]
def remove_permission(context, args):
    resource_group = context.resource_groups.get(args.resource_group)
    resources = resource_group.template.get('Resources', {})

    resource = resources.get(args.resource)
    if not resource:
        raise HandledError('The {} resource group defines no {} resource.'.format(args.resource_group, args.resource))

    permissions = resource.get('Metadata', {}).get('CloudCanvas', {}).get('Permissions', [])
    if not isinstance(permissions, list):
        permissions = [permissions]

    permission_for_role = None
    for permission in permissions:

        abstract_roles = permission.setdefault('AbstractRole', [])
        if not isinstance(abstract_roles, list):
            abstract_roles = [abstract_roles]
            permission['AbstractRole'] = abstract_roles

        if args.role in abstract_roles:

            if permission_for_role:
                raise HandledError(
                    'The {} resource {} has more than one permissions metadata object for role {}. The entry cannot be modified using the command line tool. "\
                    "Please edit the definition in the {} template file instead'.format(
                        args.resource_group,
                        args.resource,
                        args.role,
                        resource_group.template_path
                    )
                )

            permission_for_role = permission

    if not permission_for_role:
        raise HandledError('The {} resource {} defines no permissions for role {}.'.format(args.resource_group, args.resource, args.role))

    # If the entry is for other roles as well, they can't modify it using the cli.
    abstract_roles = permission_for_role['AbstractRole']
    if len(abstract_roles) > 1:
        raise HandledError(
            'The {} resource {} permissions for role {} contains permissions for more than one role ({}). "\
            "The entry cannot be modified using the command line tool. Please edit the definition in the {} template file instead'.format(
                args.resource_group,
                args.resource,
                args.role,
                ', '.join(abstract_roles),
                resource_group.template_path
            )
        )

    if args.action or args.suffix:

        if args.action:

            existing_actions = permission_for_role.setdefault('Action', [])
            if not isinstance(existing_actions, list):
                existing_actions = [existing_actions]
                permission_for_role['Action'] = existing_actions

            for action in args.action:
                if action not in existing_actions:
                    raise HandledError(
                        'The {} resource {} permissions for role {} do not include the action {}.'.format(
                            args.resource_group,
                            args.resource,
                            args.role,
                            action
                        )
                    )

            # if we remove the last action, also remove the entry for the role.
            existing_actions = [entry for entry in existing_actions if entry not in args.action]
            if existing_actions:
                permission_for_role['Action'] = existing_actions
            else:
                permissions.remove(permission_for_role)

        if args.suffix and permission_for_role in permissions:

            existing_suffixes = permission_for_role.setdefault('ResourceSuffix', [])
            if not isinstance(existing_suffixes, list):
                existing_suffixes = [existing_suffixes]
                permission_for_role['ResourceSuffix'] = existing_suffixes

            for suffix in args.suffix:
                if suffix not in existing_suffixes:
                    raise HandledError(
                        'The {} resource {} permissions for role {} do not include the suffix {}.'.format(
                            args.resource_group,
                            args.resource,
                            args.role,
                            suffix
                        )
                    )

            existing_suffixes = [entry for entry in existing_suffixes if entry not in args.suffix]
            if existing_suffixes:
                permission_for_role['ResourceSuffix'] = existing_suffixes
            else:
                del permission_for_role['ResourceSuffix']

    else:

        permissions.remove(permission_for_role)

    resource_group.save_template()

    context.view.permission_removed(args.resource_group, args.resource, args.role)


# permission list [--resource-group RESOURCE-GROUP-NAME] [--resource RESOURCE-NAME] [--role ABSTRACT-ROLE-NAME]
def list_permissions(context, args):
    permissions_list = []
    if args.resource_group:
        resource_group = context.resource_groups.get(args.resource_group)
        __list_permissions_for_resource_group(permissions_list, resource_group, args.resource, args.role)
    else:
        for resource_group in context.resource_groups.values():
            __list_permissions_for_resource_group(permissions_list, resource_group, args.resource, args.role)

    context.view.permission_list(permissions_list)


def __list_permissions_for_resource_group(permissions_list, resource_group, resource_filter, role_filter):
    resources = resource_group.template.get('Resources', {})
    for resource_name, resource_definition in six.iteritems(resources):
        if resource_filter and resource_name != resource_filter:
            continue
        __list_permissions_for_resource(permissions_list, resource_group.name, resource_name, resource_definition, role_filter)


def __list_permissions_for_resource(permissions_list, resource_group_name, resource_name, resource_definition, role_filter):
    permissions = resource_definition.get('Metadata', {}).get('CloudCanvas', {}).get('Permissions', [])
    if not isinstance(permissions, list):
        permissions = [permissions]

    for permission in permissions:

        abstract_roles = permission.get('AbstractRole', [])
        if not isinstance(abstract_roles, list):
            abstract_roles = [abstract_roles]

        if role_filter and role_filter not in abstract_roles:
            continue

        abstract_roles.sort()
        roles = ', '.join(abstract_roles)

        actions = permission.get('Action', [])
        if not isinstance(actions, list):
            actions = [actions]
        actions.sort()
        actions = ', '.join(actions)

        suffixes = permission.get('ResourceSuffix', [])
        if not isinstance(suffixes, list):
            suffixes = [suffixes]
        suffixes.sort()
        suffixes = ', '.join(suffixes)

        permissions_list.append(
            {
                'ResourceGroup': resource_group_name,
                'ResourceName': resource_name,
                'ResourceType': resource_definition.get('Type', ''),
                'Roles': roles,
                'Actions': actions,
                'Suffixes': suffixes
            }
        )


def __output_message(message, should_log=True):
    if should_log:
        print(message)


def run_project_patcher(context, args):
    """patcher patch [--silent-patch True|False --dryrun True|False --identifier "identifier"]"""
    dry_run = args.dryrun
    deployment_name = args.deployment
    should_log = not args.silent_patch
    identifier = args.identifier
    __output_message("Running lmbr patcher. Called with args: {}".format(args), should_log)
    run_project_patcher_internal(context, identifier, dry_run, should_log=should_log, deployment_name=deployment_name)


def run_project_patcher_internal(context, identifier, dry_run, should_log, deployment_name=None):
    """Internal call point for post project update"""
    # Patch project stack if it exists
    if identifier is None:
        # Select default patch
        identifier = DEFAULT_PATCH_IDENTIFIER

    if identifier == DEFAULT_PATCH_IDENTIFIER:
        __run_032020_project_patch(context, dry_run, should_log, deployment_name)
    else:
        __output_message("No patch selected", should_log)


def __run_032020_project_patch(context, dry_run, should_log, deployment_name=None):
    # Need a project stack to do any work
    if context.config.local_project_settings.project_stack_exists():
        region = context.config.project_region

        if deployment_name is None:
            project_identity_pools, project_user_pools = IdentityPoolUtils.list_cognito_pools_in_template(
                context.config.project_template_aggregator.effective_template)
            __patch_cloudcanvas_related_identity_roles(context, context.config.project_stack_id, region, project_identity_pools, dry_run, should_log)

        # Patch nominated deployment stack
        else:
            deployment_identity_pools, deployment_user_pools = \
                IdentityPoolUtils.list_cognito_pools_in_template(context.config.deployment_access_template_aggregator.effective_template)
            __patch_cloudcanvas_related_identity_roles(context, context.config.get_deployment_access_stack_id(deployment_name), region,
                                                       deployment_identity_pools, dry_run, should_log)

            # It can take up a while for the updated role policy to be propagated
            # Boto3 doesn't provide a waiter to check whether the policy has taken effect and APIs can retrieve the updated policy immediately
            # Sleep for 60s here for propagating the DENY->ALLOW flip
            time.sleep(60)
    else:
        __output_message("No active project found. Nothing todo", should_log)


def __patch_cloudcanvas_related_identity_roles(context, stack_id, region, identity_pools, dry_run, should_log):
    """
    Patches any role associated with a custom CognitoIdentityPool to ensure roles correctly federates from Cognito
    :param context: The current context object
    :param stack_id: The stack id the pool is in
    :param region: The region the stack is in
    :param identity_pools: The identity pools to find and patch roles for
    :return:
    """
    identity_client = context.aws.client("cognito-identity", region=region)
    iam_client = context.aws.client("iam", region=region)

    for pool_rec in identity_pools:
        pool_name = pool_rec.get('Name')
        pool_definition = pool_rec.get('Definition')
        __output_message("Fixing Cognito federated access for identity_pool: {}".format(pool_name), should_log)
        identity_pool_id = custom_resource_utils.get_embedded_physical_id(context.stack.get_physical_resource_id(stack_id, pool_name))

        # Get roles that are associated with identity pool. Roles that have secondary association with the pool, mostly expected to be CloudGemPortal roles
        metadata = pool_definition.get('Metadata', {})
        cc_metadata = metadata.get('CloudCanvas', {})
        role_metadata = cc_metadata.get('AdditionalAssumableRoles', [])

        # Get directly managed role names
        roles = IdentityPoolUtils.get_identity_pool_roles(identity_pool_id, identity_client)

        # Combine with roles managed / associated via metadata (secondary roles for user pools etc)
        for role_name in role_metadata:
            iam_role_name = context.stack.get_physical_resource_id(stack_id, role_name, optional=False, expected_type='AWS::IAM::Role')
            roles.append(iam_role_name)

        for role_name in roles:
            context.view.updating_role(role_name)
            role = aws_iam_role.IAMRole.factory(role_name, iam_client)
            __output_message("Checking assume role policies on IAM role: {}".format(role.arn), should_log)
            new_policy, update = IdentityPoolUtils.add_pool_to_assume_role_policy(identity_pool_id, role.assume_role_policy)
            if update:
                if should_log:
                    __output_message("{}Updating role policy to: {}".format("[DRY-RUN]: " if dry_run else "", new_policy), should_log)
                if not dry_run:
                    role.update_trust_policy(json.dumps(new_policy), iam_client)
                else:
                    __output_message("Role policy does not require update", should_log)


class IdentityPoolUtils:
    # IAM Principal used by Cognito Identity
    COGNITO_IDENTITY_PRINCIPAL = 'cognito-identity.amazonaws.com'
    # Cognito Aud key to be used in relation with STS IAM conditions
    COGNITO_AUD_CONDITION_KEY = 'cognito-identity.amazonaws.com:aud'

    """A collection of tools to help secure CloudCanvas IdentityPools"""

    def __init__(self):
        pass

    @staticmethod
    def get_identity_pool_roles(identity_pool_id, cognito_client):
        roles = []
        _response = cognito_client.get_identity_pool_roles(IdentityPoolId=identity_pool_id)
        _roles = _response.get('Roles', {})
        authenticated_role = _roles.get('authenticated', None)
        if authenticated_role:
            roles.append(aws_iam_role.IAMRole.role_name_from_arn_without_path(authenticated_role))

        unauthenticated_role = _roles.get('unauthenticated', None)
        if unauthenticated_role:
            roles.append(aws_iam_role.IAMRole.role_name_from_arn_without_path(unauthenticated_role))
        return roles

    @staticmethod
    def find_existing_pool_ids_references_in_assume_role_policy(policy_document):
        # Find if an existing condition exists in policy statement for the identity_pool_id
        # See https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_policies_iam-condition-keys.html
        # {
        #  "Effect": "Allow",
        #  "Principal": {
        #    "Federated": "cognito-identity.amazonaws.com"
        #  },
        #  "Action": "sts:AssumeRoleWithWebIdentity",
        #  "Condition": {
        #    "StringEquals": {
        #      "cognito-identity.amazonaws.com:aud": [
        #        "us-east-1:8c7a6958-4382-4a99-895c-1b326351ec5b",
        #        "us-east-1:8c7a6958-4382-4a99-895c-1b326351ec12",
        #        "us-east-1:8c7a6958-4382-4a99-895c-1b326351ec13"
        #      ]
        #    },
        #    "ForAnyValue:StringLike": {
        #      "cognito-identity.amazonaws.com:amr": "unauthenticated"
        #    }
        #  }
        # }"

        existing_pool_ids = []
        cognito_federation_statement = None
        cognito_aud_condition = None

        statements = policy_document.get("Statement", [])
        for statement in statements:
            if statement.get('Action') != "sts:AssumeRoleWithWebIdentity":
                continue

            principal = statement.get("Principal", {})
            if "Federated" in principal:
                service = principal['Federated']
                if service == IdentityPoolUtils.COGNITO_IDENTITY_PRINCIPAL:
                    cognito_federation_statement = statement
                    conditions = statement.get('Condition', {})
                    for condition in six.iteritems(conditions):
                        if 'StringEquals' in condition:
                            cognito_aud_condition = conditions['StringEquals']
                            cognito_identity_aud = cognito_aud_condition.get(IdentityPoolUtils.COGNITO_AUD_CONDITION_KEY, '')
                            if len(cognito_identity_aud) > 0:
                                existing_pool_ids = cognito_identity_aud
                                break
        return existing_pool_ids, cognito_federation_statement, cognito_aud_condition

    @staticmethod
    def add_pool_to_assume_role_policy(identity_pool_id, policy_document):
        """
        Add an identity pool to an AssumeRolePolicy statement
        - Only adds the pool if the role federates to Cognito identities via  AssumeRoleWithWebIdentity
        - Ensures that the pool identity is only added if its not in the standard aud condition

        :param identity_pool_id: The pool_id to add
        :param policy_document: Policy document to update
        :return: The new document, boolean to see if it requires update
        """
        existing_pool_ids, cognito_federation_statement, cognito_aud_condition = \
            IdentityPoolUtils.find_existing_pool_ids_references_in_assume_role_policy(policy_document)

        update = False

        # Is there a Cognito federation statement?
        if cognito_federation_statement:
            permission = cognito_federation_statement.get('Effect')

            # Enable Cognito role permissions that may have been denied during project update
            # Assumes all roles should be active (either attached to pool or in AdditionalAssumableRoles)
            if permission == 'Deny':
                cognito_federation_statement['Effect'] = 'Allow'
                update = True

            # No existing aud statement, so add one
            if cognito_aud_condition is None:
                new_condition = IdentityPoolUtils.generate_cognito_identity_condition_statement(identity_pool_id)
                if cognito_federation_statement.get('Condition') is None:
                    cognito_federation_statement['Condition'] = {}
                cognito_federation_statement.get('Condition')['StringEquals'] = new_condition
                update = True
            # Else check to see if pool_id is not in Condition
            elif identity_pool_id not in existing_pool_ids:
                new_ids = [identity_pool_id]
                # See if we have an array or a string in condition
                if type(existing_pool_ids) is list:
                    new_ids.extend(existing_pool_ids)
                else:
                    new_ids.append(existing_pool_ids)

                cognito_aud_condition[IdentityPoolUtils.COGNITO_AUD_CONDITION_KEY] = new_ids
                update = True

        return policy_document, update

    @staticmethod
    def generate_cognito_identity_condition_statement(identity_pool_id):
        """Make a new Cognito aud dictionary"""
        return {
            IdentityPoolUtils.COGNITO_AUD_CONDITION_KEY: [
                identity_pool_id
            ]
        }

    @staticmethod
    def list_cognito_pools_in_template(template):
        identity_pool_mappings = []
        user_pool_mappings = []

        for resource, definition in six.iteritems(template.get('Resources', {})):
            if definition.get('Type') == 'Custom::CognitoIdentityPool':
                identity_pool_mappings.append({'Name': resource, 'Definition': definition})
            elif definition.get('Type') == 'Custom::CognitoUserPool':
                user_pool_mappings.append({'Name': resource, 'Definition': definition})

        return identity_pool_mappings, user_pool_mappings
