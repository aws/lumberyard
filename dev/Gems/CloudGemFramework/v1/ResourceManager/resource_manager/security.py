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

from errors import HandledError

import collections

def add_cli_commands(subparsers, add_common_args):
    __add_role_cli_commands(subparsers, add_common_args)
    __add_role_mapping_cli_commands(subparsers, add_common_args)
    __add_permission_cli_commands(subparsers, add_common_args)


def __add_role_cli_commands(subparsers, add_common_args):

    parser = subparsers.add_parser('role', help='Add, modify, and remove AWS IAM Role definitions in the project-template.json and deployment-access-template.json files.')
    subparsers = parser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    # role add [--project] --role ROLE-NAME
    subparser = subparsers.add_parser('add', help='Adds an AWS IAM Role definition to the project-template.json or deployment-access-template.json files.')
    subparser.add_argument('--project', action='store_true', help='Indicates that the role definition should go in the project-template.json file. The default is to put it in the deployment-access-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ROLE-NAME', help='The name of the role definition.')
    add_common_args(subparser)
    subparser.set_defaults(func=add_role)
    
    # role remove [--project] --role ROLE-NAME
    subparser = subparsers.add_parser('remove', help='Removes an AWS IAM Role definition from the project-template.json or deployment-access-template.json files.')
    subparser.add_argument('--project', action='store_true', help='Indicates that the role definition should be removed from the project-template.json file. The default is to remove it from the deployment-access-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ROLE-NAME', help='The name of the role definition.')
    add_common_args(subparser)
    subparser.set_defaults(func=remove_role)
    
    # role list [--project|--deployment]
    subparser = subparsers.add_parser('list', help='Lists the AWS IAM Role definitions in the project-template.json and deployment-access-template.json files.')
    group = subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('--project', action='store_true', help='Lists role definitions in the project-template.json file. The default is to list definitions in the project-template.json and deployment-access-template.json files.')
    group.add_argument('--deployment', action='store_true', help='Lists role definitions in the deployment-access-template.json file. The default is to list role definitions in the project-template.json and deployment-access-template.json files.')
    add_common_args(subparser)
    subparser.set_defaults(func=list_roles)
    

def __add_role_mapping_cli_commands(subparsers, add_common_args):

    parser = subparsers.add_parser('role-mapping', help='Add, modify, and remove Cloud Canvas RoleMappings metadata on AWS IAM Role definitions in the project-template.json and deployment-access-template.json files.')
    subparsers = parser.add_subparsers(dest='subparser_name')

    # role-mapping add [--project] --role ROLE-NAME --pattern ABSTRACT-ROLE-PATTERN --allow|--deny
    subparser = subparsers.add_parser('add', help='Adds Cloud Canvas RoleMappings metadata to an AWS IAM Role definition in the project-template.json or deployment-access-template.json file.')
    subparser.add_argument('--project', action='store_true', help='Indicates that the role definition is in the project-template.json file. The default is for the role definition to be in the deployment-access-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ROLE-NAME', help='The name of the role definition.')
    subparser.add_argument('--pattern', metavar='ABSTRACT-ROLE-PATTERN', required=True, help='Identifies the abstract roles mapped to the role. Has the form <resource-group-name>.<abstract-role-name>, where <resource-group-name> can be *.')
    group = subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('--allow', action='store_true', help='Indicates that the permissions requested for the abstract role are allowed.')
    group.add_argument('--deny', action='store_true', help='Indicates that the permissions requested for the abstract role are denied.')
    add_common_args(subparser)
    subparser.set_defaults(func=add_role_mapping)
    
    # role-mapping remove [--project] --role ROLE-NAME --pattern ABSTRACT-ROLE-PATTERN
    subparser = subparsers.add_parser('remove', help='Removes Cloud Canvas RoleMappings metadata from an AWS IAM Role definition in the project-template.json or deployment-access-template.json file.')
    subparser.add_argument('--project', action='store_true', help='Indicates that the role definition is in the project-template.json file. The default is for the role definition to be in the deployment-access-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ROLE-NAME', help='The name of the role definition.')
    subparser.add_argument('--pattern', required=True, metavar='ABSTRACT-ROLE-PATTERN', help='Identifies the abstract roles mapped to the role. Has the form <resource-group-name>.<abstract-role-name>, where <resource-group-name> can be *.')
    add_common_args(subparser)
    subparser.set_defaults(func=remove_role_mapping)

    # role-mapping list [--project|--deployment] [--role ROLE-NAME] [--pattern ABSTRACT-ROLE-PATTERN]
    subparser = subparsers.add_parser('list', help='Lists Cloud Canvas RoleMappings metadata on the AWS IAM Role definitions in the project-template.json and deployment-access-template.json files.')
    group = subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('--project', action='store_true', help='Lists metadata from role definitions in the project-template.json file. The default is to list metadata from role definitions in the project-template.json and deployment-access-template.json files.')
    group.add_argument('--deployment', action='store_true', help='Lists metadata from role definitions in the deployment-access-template.json file. The default is to list metadata from role definitions in the project-template.json and deployment-access-template.json files.')
    subparser.add_argument('--role', metavar='ROLE-NAME', required=False, help='The role definition with the metadata to list. The default is to list metadata from all role definitions.')
    subparser.add_argument('--pattern', metavar='ABSTRACT-ROLE-PATTERN', help='The abstract role pattern specified by the metadata listed. The default is to list metadata with any abstract role pattern.')
    add_common_args(subparser)
    subparser.set_defaults(func=list_role_mappings)


def __add_permission_cli_commands(subparsers, add_common_args):

    parser = subparsers.add_parser('permission', help='Add, modify, and remove Cloud Canvas Permissions mapping metadata on resource definitions in resource-group-template.json files.')
    subparsers = parser.add_subparsers(dest='subparser_name')

    # permission add --resource-group RESOURCE-GROUP-NAME --resource RESOURCE-NAME --role ABSTRACT-ROLE-NAME --action ACTION ... [--suffix SUFFIX ...]
    subparser = subparsers.add_parser('add', help='Adds Cloud Canvas Permissions metadata to an resource definition in a resource-group-template.json file.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP-NAME', required=True, help='The name of a resource group. The metadata will be added to a resource definition in that resource group\'s resource-group-template.json file.')
    subparser.add_argument('--resource', metavar='RESOURCE-NAME', required=True, help='The name of the resource definition in the resource-group-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ABSTRACT-ROLE-NAME', help='Identifies the role that will be granted the permission.')
    subparser.add_argument('--action', required=False, nargs='+', metavar='ACTION', help='The action that is allowed. May be specified more than once.')
    subparser.add_argument('--suffix', required=False, nargs='+', metavar='SUFFIX', help='A string appeneded to the resoure ARN. May be specified more than once.')
    add_common_args(subparser)
    subparser.set_defaults(func=add_permission)

    # permission remove --resource-group RESOURCE-GROUP-NAME --resource RESOURCE-NAME --role ABSTRACT-ROLE-NAME ... [--action ACTION ...] [--suffix SUFFIX ...]
    subparser = subparsers.add_parser('remove', help='Removes Cloud Canvas Permissions metadata from a resource definition in a resource-group-template.json file.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP-NAME', required=True, help='The name of a resource group. The metadata will be removed from a resource definition in that resource group\'s resource-group-template.json file.')
    subparser.add_argument('--resource', metavar='RESOURCE-NAME', required=True, help='The name of the resource definition in the resource-group-template.json file.')
    subparser.add_argument('--role', required=True, metavar='ABSTRACT-ROLE-NAME', help='Identifies the roles from which permissions are removed.')
    subparser.add_argument('--action', required=False, nargs='+', metavar='ACTION', help='The action that is removed. May be specified more than once.')
    subparser.add_argument('--suffix', required=False, nargs='+', metavar='SUFFIX', help='A string appeneded to the resoure ARN, which is removed. May be specified more than once.')
    add_common_args(subparser)
    subparser.set_defaults(func=remove_permission)

    # permission list [--resource-group RESOURCE-GROUP-NAME] [--resource RESOURCE-NAME] [--role ABSTRACT-ROLE-NAME]
    subparser = subparsers.add_parser('list', help='Removes Cloud Canvas Permissions metadata from a resource definition in a resource-group-template.json file.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP-NAME', help='Will list the metadata from resource definitions in the resource group\'s resource-group-template.json file. The default is to list permissions from all resource groups.')
    subparser.add_argument('--resource', metavar='RESOURCE-NAME', help='The name of the resource definition in the resource-group-template.json file. The default is to list metadata from all resource definitions.')
    subparser.add_argument('--role', metavar='ABSTRACT-ROLE-NAME', help='Lists matadata for the specified abstract role. The default is to list metadata for all abstract roles.')
    add_common_args(subparser)
    subparser.set_defaults(func=list_permissions)


DEFAULT_ACCESS_CONTROL_RESOURCE_DEFINITION = {
    "Type": "Custom::AccessControl",
    "Properties": {
        "ServiceToken": { "Ref": "ProjectResourceHandler" },
        "ConfigurationBucket": { "Ref": "ConfigurationBucket" },
        "ConfigurationKey": { "Ref": "ConfigurationKey" }
    },
    "DependsOn": []
}


def ensure_access_control(template, resource):
    '''Make sure an AccessControl resource exists and that it depends on a resource.
    
    Args:
    
        template - the template to check
        
        resource - the name of the resource
        
    Returns:
    
        True if the resource was added, False if it was already there.
    ''' 

    changed = False

    resources = template.setdefault('Resources', {})

    access_control = resources.setdefault('AccessControl', DEFAULT_ACCESS_CONTROL_RESOURCE_DEFINITION)

    dependencies = access_control.setdefault('DependsOn', [])
    if not isinstance(dependencies, list):
        dependencies = [ dependencies ]
        access_control['DependsOn'] = dependencies

    if resource not in dependencies:
        dependencies.append(resource)
        changed = True

    return changed


def ensure_no_access_control(template, resource):
    '''Make sure the AccessControl resource, if it exists, does not depend on a resource.
    
    Args:
    
        template - the template to check
        
        resource - the name of the resource
        
    Returns:
    
        True if the resource was removed, False if it wasn't there.
    ''' 

    resources = template.get('Resources', {})

    access_control = resources.get('AccessControl', {})

    dependencies = access_control.get('DependsOn', [])
    if isinstance(dependencies, list) and resource in dependencies:
        dependencies.remove(resource)
        changed = True
    elif isinstance(dependencies, basestring) and resource == dependencies:
        access_control['DependsOn'] = []
        changed = True

    return changed

TargetTemplate = collections.namedtuple('TargetTemplate', 'template file_path save role_path scope')


def __get_target_template(context, args):

    if args.project:
        return TargetTemplate(
            template = context.config.project_template_aggregator.extension_template,
            file_path = context.config.project_template_aggregator.extension_file_path,
            save = context.config.project_template_aggregator.save_extension_template,
            role_path = { "Fn::Join": [ "", [ "/", { "Ref": "AWS::StackName" }, "/" ]] },
            scope = 'project')
    else:
        return TargetTemplate(
            template = context.config.deployment_template_aggregator.extension_template,
            file_path = context.config.deployment_template_aggregator.extension_file_path,
            save = context.config.deployment_template_aggregator.save_extension_template,
            role_path = { "Fn::Join": [ "", [ "/", { "Ref": "ProjectStack" }, "/", { "Ref": "DeploymentName" }, "/" ]] },
            scope = 'deployment')


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
                        "Principal": { "AWS": {"Ref": "AWS::AccountId"} }
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
        raise HandledError('Resource {} in the {} template has type {} not type AWS::IAM::Role.'.format(args.role, target.file_path,resources[args.role].get('Type', '(none)')))

    if args.role in ['Player', 'DeploymentAdmin', 'DeploymentOwner', 'ProjectAdmin', 'ProjectOwner', 'ProjectResourceHandlerExecution', 'PlayerAccessTokenExchangeExecution']:
        raise HandledError('The {} role is required by Cloud Canvas and cannot be removed.'.format(args.role))

    del resources[args.role]

    ensure_no_access_control(target.template, args.role)

    target.save()

    context.view.role_removed(target.scope, args.role)


# role list [--project|--deployment]
def list_roles(context, args):

    role_list = []

    if not args.deployment:
        role_list.extend(__list_roles_in_template(context.config.extended_project_template.effective_template, 'Project'))

    if not args.project:
        role_list.extend(__list_roles_in_template(context.config.extended_deployment_access_template.effective_template, 'Deployment'))

    context.view.role_list(role_list)


def __list_roles_in_template(template, scope):

    role_list = []

    for resource, definition in template.get('Resources', {}).iteritems():
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
        raise HandledError('Resource {} in the {} template has type {} not type AWS::IAM::Role.'.format(args.role, target.file_path, role_resource.get('Type', '(none)')))

    _validate_abstract_role_pattern(args.pattern)

    effect = 'Allow' if args.allow else 'Deny'

    role_mappings_metadata = role_resource.setdefault('Metadata', {}).setdefault('CloudCanvas', {}).setdefault('RoleMappings', [])

    if not isinstance(role_mappings_metadata, list):
        role_mappings_metadata = [ role_mappings_metadata ]
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
        raise HandledError('Resource {} in the {} template has type {} not type AWS::IAM::Role.'.format(args.role, target.file_path, role_resource.get('Type', '(none)')))

    _validate_abstract_role_pattern(args.pattern)

    role_mappings_metadata = role_resource.setdefault('Metadata', {}).setdefault('CloudCanvas', {}).setdefault('RoleMappings', [])

    if not isinstance(role_mappings_metadata, list):
        role_mappings_metadata = [ role_mappings_metadata ]
        role_resource['Metadata']['CloudCanvas']['RoleMappings'] = role_mappings_metadata

    new_role_mappings_metadata = [ entry for entry in role_mappings_metadata if entry.get('AbstractRole') != args.pattern ]

    if len(new_role_mappings_metadata) == len(role_mappings_metadata):
        raise HandledError('No role mapping for {} was found on the role {} in {}.'.format(args.pattern, args.role, target_template_path))

    role_resource['Metadata']['CloudCanvas']['RoleMappings'] = new_role_mappings_metadata

    target.save()

    context.view.role_mapping_removed(target.scope, args.role, args.pattern)


# role-mapping list [--project|--deployment] [--role ROLE-NAME] [--pattern ABSTRACT-ROLE-PATTERN]
def list_role_mappings(context, args):

    role_mapping_list = []

    if not args.deployment:
        role_mapping_list.extend(__list_role_mappings_in_template(context.config.extended_project_template.effective_template, 'Project'))

    if not args.project:
        role_mapping_list.extend(__list_role_mappings_in_template(context.config.extended_deployment_access_template.effective_template, 'Deployment'))

    context.view.role_mapping_list(role_mapping_list)


def __list_role_mappings_in_template(template, scope):
    result = []
    for resource_name, resource_definition in template.get('Resources', {}).iteritems():
        if resource_definition.get('Type') == 'AWS::IAM::Role':
            role_mappings = resource_definition.get('Metadata', {}).get('CloudCanvas', {}).get('RoleMappings', [])
            if not isinstance(role_mappings, list):
                role_mappings = [ role_mappings ]
            for role_mapping in role_mappings:
                patterns = role_mapping.get('AbstractRole', [])
                if not isinstance(patterns, list):
                    patterns = [ patterns ]
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
        raise HandledError('The abstract role pattern {} should have the format <resource-group-name>.role-name>, where <resource-group-name> can be *.'.format(pattern))


# permission add --resource-group RESOURCE-GROUP-NAME --resource RESOURCE-NAME --role ABSTRACT-ROLE-NAME [--action ACTION ...] [--suffix SUFFIX ...]
def add_permission(context, args):
    add_permission_to_role(context, args.resource_group, args.resource, args.role, args.action, args.suffix)

def add_permission_to_role(context, target_resource_group, target_resource, role_name, actions = None, suffixes = None):
    resource_group = context.resource_groups.get(target_resource_group)
    resources = resource_group.template.get('Resources', {})

    resource = resources.get(target_resource)
    if not resource:
        raise HandledError('The {} resource group defines no {} resource.'.format(target_resource_group, target_resource))
    
    permissions = resource.setdefault('Metadata', {}).setdefault('CloudCanvas', {}).setdefault('Permissions', [])
    if not isinstance(permissions, list):
        permissions = [ permissions ]
        resource['Metadata']['CloudCanvas']['Permissions'] = permissions

    # Look for an existing entry for the abstract role. If there is more
    # than one, then the cli can't be used to edit them.

    permission_for_role = None
    for permission in permissions:
        
        abstract_roles = permission.setdefault('AbstractRole', [])
        if not isinstance(abstract_roles, list):
            abstract_roles = [ abstract_roles ]
            permission['AbstractRole'] = abstract_roles

        if role_name in abstract_roles:

            if permission_for_role:
                raise HandledError(
                    'The {} resource {} has more than one permissions metadata object for role {}. The entry cannot be modified using the command line tool. Please edit the definition in the {} template file instead'.format(
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
                'The {} resource {} permissions for role {} contains permissions for more than one role ({}). The entry cannot be modified using the command line tool. Please edit the definition in the {} template file instead'.format(
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
                existing_actions = [ existing_actions ]
                permission_for_role['Action'] = existing_actions

            for action in actions:
                if action in existing_actions:
                    raise HandledError('The {} resource {} permissions for role {} already specify action {}.'.format(target_resource_group, target_resource, role_name, action))
                existing_actions.append(action)

        # If a suffix was given, add it to the list of suffixes.
        if suffixes:

            existing_suffixes = permission.setdefault('ResourceSuffix', [])
            if not isinstance(existing_suffixes, list):
                existing_suffixes = [ existing_suffixes ]
                permissions['ResoureSuffix'] = existing_suffixes

            for suffix in suffixes:
                if suffix in existing_suffixes:
                    raise HandledError('The {} resource {} permissions for role {} aleady specify suffix {}.'.format(target_resource_group, target_resource, role_name, suffix))
                existing_suffixes.append(suffix)


    else:

        # There is no existing entry for the abstract role...

        # An action must be specified.
        if not actions:
            raise HandledError('No {} resource {} permissions for role {} are defined. The --action option must be specified.'.format(target_resource_group, target_resource, role_name))

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
        permissions = [ permissions ]

    permission_for_role = None
    for permission in permissions:
        
        abstract_roles = permission.setdefault('AbstractRole', [])
        if not isinstance(abstract_roles, list):
            abstract_roles = [ abstract_roles ]
            permission['AbstractRole'] = abstract_roles

        if args.role in abstract_roles:

            if permission_for_role:
                raise HandledError(
                    'The {} resource {} has more than one permissions metadata object for role {}. The entry cannot be modified using the command line tool. Please edit the definition in the {} template file instead'.format(
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
            'The {} resource {} permissions for role {} contains permissions for more than one role ({}). The entry cannot be modified using the command line tool. Please edit the definition in the {} template file instead'.format(
                args.resource_group,
                args.resource,
                args.role,
                ', '.join(abstract_roles),
                resource_group.template_path
            )
        )

    if (args.action or args.suffix):
               
        if args.action:

            existing_actions = permission_for_role.setdefault('Action', [])
            if not isinstance(existing_actions, list):
                existing_actions = [ existing_actions ]
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
            existing_actions = [ entry for entry in existing_actions if entry not in args.action ]
            if existing_actions:
                permission_for_role['Action'] = existing_actions
            else:
                permissions.remove(permission_for_role)

        if args.suffix and permission_for_role in permissions:

            existing_suffixes = permission_for_role.setdefault('ResourceSuffix', [])
            if not isinstance(existing_suffixes, list):
                existing_suffixes = [ existing_suffixes ]
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

            existing_suffixes = [ entry for entry in existing_suffixes if entry not in args.suffix ]
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
    for resource_name, resource_definition in resources.iteritems():
        if resource_filter and resource_name != resource_filter:
            continue
        __list_permissions_for_resource(permissions_list, resource_group.name, resource_name, resource_definition, role_filter)


def __list_permissions_for_resource(permissions_list, resource_group_name, resource_name, resource_definition, role_filter):
    
    permissions = resource_definition.get('Metadata', {}).get('CloudCanvas', {}).get('Permissions', [])
    if not isinstance(permissions, list):
        permissions = [ permissions ]

    for permission in permissions:
        
        abstract_roles = permission.get('AbstractRole', [])
        if not isinstance(abstract_roles, list):
            abstract_roles = [ abstract_roles ]

        if role_filter and role_filter not in abstract_roles:
            continue

        abstract_roles.sort()
        roles = ', '.join(abstract_roles)

        actions = permission.get('Action', [])
        if not isinstance(actions, list):
            actions = [ actions ]
        actions.sort()
        actions = ', '.join(actions)

        suffixes = permission.get('ResourceSuffix', [])
        if not isinstance(suffixes, list):
            suffixes = [ suffixes ]
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
