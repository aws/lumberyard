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

from errors import HandledError

import os
import util

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

    if args.release:
        __update_release(context, args)
    elif args.deployment:
        __update_launcher_with_deployment(context, args)
    else:
        __update_default(context, args)


def __update_default(context, args):
    default_mappings = {}

    if context.config.default_deployment is not None:
        default_mappings = __get_mappings(context, context.config.default_deployment)

    context.config.set_user_mappings(default_mappings)

def __update_release(context, args):
    if context.config.release_deployment is not None:
        __update_launcher(context, context.config.release_deployment)
    else:
        print 'Release deployment is not currently set.'

def __update_launcher_with_deployment(context, args):
    deployment_name = args.deployment
    if not deployment_name in context.config.deployment_names:
        raise HandledError('The project has no {} deployment.'.format(deployment_name))
    __update_launcher(context, deployment_name)

def __update_launcher(context, deployment_name):
    kLogicalMappingsObjectName = 'LogicalMappings'

    logicalMappingsFileName = deployment_name + '.awsLogicalMappings.json'

    mappings = __get_mappings(context, deployment_name)

    outData = {}
    outData[kLogicalMappingsObjectName] = mappings
    outData['Protected'] = deployment_name in context.config.get_protected_depolyment_names()

    logicalMappingsPath = context.config.root_directory_path
    logicalMappingsPath = os.path.join(logicalMappingsPath, context.config.game_directory_name)
    logicalMappingsPath = os.path.join(logicalMappingsPath, 'Config')
    logicalMappingsPath = os.path.join(logicalMappingsPath, logicalMappingsFileName)

    context.config.save_json(logicalMappingsPath, outData)

def __get_mappings(context, deployment_name):

    mappings = {}

    deployment_stack_id = context.config.get_deployment_stack_id(deployment_name)

    region = util.get_region_from_arn(deployment_stack_id)
    account_id = util.get_account_id_from_arn(deployment_stack_id)

    context.view.retrieving_mappings(deployment_name, deployment_stack_id)

    player_accessible_arns = __get_player_accessible_arns(context, deployment_name)

    resources = context.stack.describe_resources(deployment_stack_id, recursive=True)

    for logical_name, description in resources.iteritems():
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
    login_exchange_handler = context.stack.get_physical_resource_id(context.config.project_stack_id, k_exchange_token_handler_name)
    if login_exchange_handler != None:
        mappings[k_exchange_token_handler_name] = { 'PhysicalResourceId': login_exchange_handler, 'ResourceType': 'AWS::Lambda::Function' }

    #now let's grab the player identity stuff and make sure we add it to the mappings.
    access_stack_arn = context.config.get_deployment_access_stack_id(deployment_name)
    if access_stack_arn != None:
        access_resources = context.stack.describe_resources(access_stack_arn, recursive=True)
        for logical_name, description in access_resources.iteritems():
            if description['ResourceType'] == 'Custom::CognitoIdentityPool':
                mappings[logical_name] = {
                    'PhysicalResourceId': description['PhysicalResourceId'],
                    'ResourceType': description['ResourceType']
                }

    mappings['region'] = { 'PhysicalResourceId': region, 'ResourceType': 'Configuration' }
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

def __get_player_accessible_arns(context, deployment_name):

    player_accessible_arns = set()

    deployment_access_stack_id = context.config.get_deployment_access_stack_id(deployment_name)
    player_role_id = context.stack.get_physical_resource_id(deployment_access_stack_id, 'Player')

    iam = context.aws.client('iam')

    res = iam.list_role_policies(RoleName=player_role_id)

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
