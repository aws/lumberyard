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
# $Revision: #3 $

from errors import HandledError

import os
import util
import constant
import glob
from botocore.exceptions import ClientError

def list(context, args):

    protected = context.config.user_default_deployment in context.config.get_protected_depolyment_names()
    list = []

    mappings = context.config.user_settings.get('Mappings', {})

    for key, value in mappings.iteritems():
        value['Name'] = key
        list.append(value)

    context.view.mapping_list(context.config.default_deployment, list, protected)


def update(context, args):

    # Has the project been initialized?
    if not context.config.project_initialized:
        raise HandledError('The project has not been initialized.')

    __remove_old_mapping_files(context)
    context.view.mapping_update(context.config.default_deployment, args)

    if args.release:
        __update_release(context, args)
    elif args.deployment:
        __update_launcher_with_deployment(context, args)
    else:
        __update_default(context, args)

def __update_default(context, args):
    default_mappings = {}
    if context.config.default_deployment is not None:
        __update_logical_mappings_files(context, context.config.default_deployment, args)

    if context.config.default_deployment is not None:
        exclusions = set(context.config.local_project_settings.get('ExcludedMappings', []))
        exclusions.update(set(context.config.local_project_settings.get('EditorExcludedMappings', [])))
        default_mappings = __get_mappings(context, context.config.default_deployment, exclusions, "Player", args)

    context.config.set_user_mappings(default_mappings)

def __update_release(context, args):
    if context.config.release_deployment is not None:
        __update_logical_mappings_files(context, context.config.release_deployment, args)
    else:
        print 'Release deployment is not currently set.'

def __update_launcher_with_deployment(context, args):
    deployment_name = args.deployment
    if not deployment_name in context.config.deployment_names:
        raise HandledError('The project has no {} deployment.'.format(deployment_name))
    __update_logical_mappings_files(context, deployment_name, args)

def __update_logical_mappings_files(context, deployment_name, args=None):
    __update_launcher(context, deployment_name, 'Player', args)
    __update_launcher(context, deployment_name, 'Server', args)

def __update_launcher(context, deployment_name, role, args=None):
    kLogicalMappingsObjectName = 'LogicalMappings'

    logicalMappingsFileName = '{}.{}.{}'.format(deployment_name, role.lower(), constant.MAPPING_FILE_SUFFIX)

    exclusions = set(context.config.local_project_settings.get('ExcludedMappings', []))
    exclusions.update(set(context.config.local_project_settings.get('LauncherExcludedMappings', [])))
    mappings = __get_mappings(context, deployment_name, exclusions, role, args)

    outData = {}
    outData[kLogicalMappingsObjectName] = mappings
    outData['Protected'] = deployment_name in context.config.get_protected_depolyment_names()

    logicalMappingsPath = __logical_mapping_file_path(context)
    logicalMappingsPath = os.path.join(logicalMappingsPath, logicalMappingsFileName)

    context.config.save_json(logicalMappingsPath, outData)

def __logical_mapping_file_path(context):
    logicalMappingsPath = context.config.root_directory_path
    logicalMappingsPath = os.path.join(logicalMappingsPath, context.config.game_directory_name)
    logicalMappingsPath = os.path.join(logicalMappingsPath, 'Config')

    return logicalMappingsPath

def __remove_old_mapping_files(context):
    logicalMappingsPath = __logical_mapping_file_path(context)
    if not os.path.exists(logicalMappingsPath):
        return
    cwd = os.getcwd()
    os.chdir(logicalMappingsPath)
    filelist = glob.glob('*.{}'.format(constant.MAPPING_FILE_SUFFIX))
    for f in filelist:
        if os.path.isdir(f):
            continue
        try:
            os.remove(f)
        except:
            pass
    os.chdir(cwd)

def __get_mappings(context, deployment_name, exclusions, role, args=None):

    mappings = {}

    deployment_stack_id = context.config.get_deployment_stack_id(deployment_name)

    region = util.get_region_from_arn(deployment_stack_id)
    account_id = util.get_account_id_from_arn(deployment_stack_id)

    context.view.retrieving_mappings(deployment_name, deployment_stack_id, role)

    player_accessible_arns = __get_player_accessible_arns(context, deployment_name, role, args)

    resources = context.stack.describe_resources(deployment_stack_id, recursive=True)

    for logical_name, description in resources.iteritems():

        if logical_name in exclusions:
            continue

        physical_resource_id = description.get('PhysicalResourceId')
        if physical_resource_id:
            if __is_user_pool_resource(description):
                mappings[logical_name] = {
                    'PhysicalResourceId': physical_resource_id,
                    'ResourceType': description['ResourceType'],
                    'UserPoolClients': description['UserPoolClients'] # include client id / secret
                }
            else:
                resource_arn = util.get_resource_arn(
                    description['StackId'],
                    description['ResourceType'],
                    physical_resource_id,
                    optional = True,
                    context = context
                )
                if resource_arn and resource_arn in player_accessible_arns:
                    if __is_service_api_resource(description):
                        __add_service_api_mapping(context, logical_name, description, mappings)
                    else:
                        mappings[logical_name] = {
                            'PhysicalResourceId': physical_resource_id,
                            'ResourceType': description['ResourceType']
                        }

    k_exchange_token_handler_name = 'PlayerAccessTokenExchange'
    if k_exchange_token_handler_name not in exclusions:
        login_exchange_handler = context.stack.get_physical_resource_id(context.config.project_stack_id, k_exchange_token_handler_name)
        if login_exchange_handler != None:
            mappings[k_exchange_token_handler_name] = { 'PhysicalResourceId': login_exchange_handler, 'ResourceType': 'AWS::Lambda::Function' }

    #now let's grab the player identity stuff and make sure we add it to the mappings.
    access_stack_arn = context.config.get_deployment_access_stack_id(deployment_name, True if args is not None and args.is_gui else False)
    if access_stack_arn != None:
        access_resources = context.stack.describe_resources(access_stack_arn, recursive=True)
        for logical_name, description in access_resources.iteritems():
            if description['ResourceType'] == 'Custom::CognitoIdentityPool':

                if logical_name in exclusions:
                    continue

                mappings[logical_name] = {
                    'PhysicalResourceId': description['PhysicalResourceId'],
                    'ResourceType': description['ResourceType']
                }

    if 'region' not in exclusions:
        mappings['region'] = { 'PhysicalResourceId': region, 'ResourceType': 'Configuration' }

    if 'account_id' not in exclusions:
        mappings['account_id'] = { 'PhysicalResourceId': account_id, 'ResourceType': 'Configuration' }

    return mappings


def __is_service_api_resource(description):
    return description.get('ResourceType', '') == 'Custom::ServiceApi'

def __is_user_pool_resource(description):
    return description.get('ResourceType', '') == 'Custom::CognitoUserPool'

def __add_service_api_mapping(context, logical_name, resource_description, mappings):
    found = False
    stack_id = resource_description.get('StackId', None)
    if stack_id:
        stack_description = context.stack.describe_stack(stack_id)
        outputs = stack_description.get('Outputs', [])
        if outputs == None:
            outputs = []
        for output in outputs:
            if output.get('OutputKey', '') == 'ServiceUrl':
                service_url = output.get('OutputValue', None)
                if service_url:
                    mappings[logical_name] = {
                        'PhysicalResourceId': service_url,
                        'ResourceType': resource_description['ResourceType']
                    }
                    found = True

    if not found:
        resource_group_name = logical_name.split('.')[0]
        raise HandledError('The {} resource group template defines a ServiceApi resource but does not provide a ServiecUrl Output value.'.format(resource_group_name))

def __get_player_accessible_arns(context, deployment_name, role, args=None):

    player_accessible_arns = set()

    deployment_access_stack_id = context.config.get_deployment_access_stack_id(deployment_name, True if args is not None and args.is_gui else False)
    player_role_id = context.stack.get_physical_resource_id(deployment_access_stack_id, role, optional=True)

    if player_role_id == None:
        return {}

    iam = context.aws.client('iam')

    try:
        res = iam.list_role_policies(RoleName=player_role_id)
    except ClientError as e:
        if e.response["Error"]["Code"] in ["AccessDenied"]:
            return {}

    for policy_name in res.get('PolicyNames', []):

        res = iam.get_role_policy(RoleName=player_role_id, PolicyName=policy_name)

        policy_document = res.get('PolicyDocument', {})

        statements = policy_document.get('Statement', [])
        if not isinstance(statements, type([])): # def list above hides list
            statements = [ statements ]

        for statement in statements:

            if statement.get('Effect', '') != 'Allow':
                continue

            resource_arns = statement.get('Resource', [])
            if not isinstance(resource_arns, type([])): # def list above hides list
                resource_arns = [ resource_arns ]

            for resource_arn in resource_arns:
                player_accessible_arns.add(__trim_player_accessible_arn(resource_arn))

    return player_accessible_arns

def __trim_player_accessible_arn(arn):

    # Remove any suffix from arns that have them, we want only the base are
    # so it matches the arn generated from the physical resource id later.

    if arn.startswith('arn:aws:execute-api:'):
        return util.trim_at(arn, '/')

    if arn.startswith('arn:aws:s3:'):
        return util.trim_at(arn, '/')

    return arn
