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
import json
import util
import glob
from resource_manager_common import constant
from botocore.exceptions import ClientError
from boto3.exceptions import S3UploadFailedError

from cgf_utils import aws_utils
from cgf_utils import custom_resource_utils
from resource_manager_common import resource_type_info
from resource_manager_common import stack_info

_MAPPINGS_KEY_PREFIX = "mappings/"

def list(context, args):

    protected = context.config.user_default_deployment in context.config.get_protected_depolyment_names()
    list = []

    mappings = context.config.user_settings.get('Mappings', {})

    for key, value in mappings.iteritems():
        value['Name'] = key
        list.append(value)

    context.view.mapping_list(context.config.default_deployment, list, protected)

def force_update(context, args):
    update(context, args, True)

def update(context, args, force=False):
    # Has the project been initialized?
    if not context.config.project_initialized:
        raise HandledError('The project has not been initialized.')

    if args.release:
        args.deployment = context.config.release_deployment

    if not args.deployment:
        args.deployment = context.config.default_deployment

    if args.deployment is None:
        return

    s3_client = context.aws.client('s3')
    config_bucket = context.config.configuration_bucket_name

    player_mappings_file_name = '{}.{}.{}'.format(args.deployment, 'player', constant.MAPPING_FILE_SUFFIX)
    player_mappings_file_path = os.path.join(__logical_mapping_file_path(context), player_mappings_file_name)

    server_mappings_file_name = '{}.{}.{}'.format(args.deployment, 'server', constant.MAPPING_FILE_SUFFIX)
    server_mappings_file_path = os.path.join(__logical_mapping_file_path(context), server_mappings_file_name)

    mappings_file_names = (player_mappings_file_name, server_mappings_file_name)
    mappings_file_paths = (player_mappings_file_path, server_mappings_file_path)

    if not args.ignore_cache:
        # See if we can just grab the mappings files from s3 instead of reprocessing them.
        force = False  # If everything goes well we can just use the files we download.

        # Obtain the most recent update time for the stack
        deployment_stack_id = context.config.get_deployment_stack_id(args.deployment)
        last_update_time = context.stack.describe_stack(deployment_stack_id)['LastUpdatedTime']

        # List the mappings file available in the config bucket
        items = s3_client.list_objects_v2(Bucket=config_bucket, Prefix=_MAPPINGS_KEY_PREFIX)
        existing_s3_keys = {item['Key']: item for item in items.get('Contents', [])}

        # For both types of mapping files, check if a corresponding file exists in S3 and if it's newer than our
        # current deployment. If either of them is missing or out-of-date, force the mappings to update manually.
        for file_name, file_path in zip(mappings_file_names, mappings_file_paths):
            s3_entry = existing_s3_keys.get(_MAPPINGS_KEY_PREFIX + file_name, None)

            if not s3_entry or (last_update_time and s3_entry['LastModified'] <= last_update_time):
                force = True
                break

            s3_client.download_file(config_bucket, _MAPPINGS_KEY_PREFIX + file_name, file_path)

    if force or not (os.path.exists(player_mappings_file_path) and os.path.exists(server_mappings_file_path)):
        # __update_with_deployment will update the user settings if it is the default deployment
        __update_with_deployment(context, args)

        try:
            # Upload the resultant files to s3 both for caching purposes and for non-admin users to download
            for file_name, file_path in zip(mappings_file_names, mappings_file_paths):
                s3_client.upload_file(file_path, config_bucket, _MAPPINGS_KEY_PREFIX + file_name)
        except S3UploadFailedError as e:
            print "This client does not have upload permissions for the project configuration bucket. skipping upload"

    elif args.deployment == context.config.default_deployment:
        # we didn't need to get the mappings from the backend, update the user settings ourselves from local mappings
        __update_user_mappings_from_file(context, player_mappings_file_path)

    set_launcher_deployment(context, context.config.release_deployment or context.config.default_deployment or args.deployment)


def set_launcher_deployment(context, deployment_name):
    player_mappings_file = os.path.join(__logical_mapping_file_path(context), '{}.{}.{}'.format(deployment_name, 'player', constant.MAPPING_FILE_SUFFIX))
    server_mappings_file = os.path.join(__logical_mapping_file_path(context), '{}.{}.{}'.format(deployment_name, 'server', constant.MAPPING_FILE_SUFFIX))
    if not (os.path.exists(player_mappings_file) and os.path.exists(server_mappings_file)):
        __update_logical_mappings_files(context, deployment_name)
    launcher_deployment_file = os.path.join(__logical_mapping_file_path(context), "launcher.deployment.json")
    if not deployment_name:
        os.remove(launcher_deployment_file)
        return

    with open(launcher_deployment_file, 'w') as out_file:
        json.dump({"LauncherDeployment" : deployment_name}, out_file)

def __update_with_deployment(context, args):
    deployment_name = args.deployment
    if not deployment_name in context.config.deployment_names:
        raise HandledError('The project has no {} deployment.'.format(deployment_name))
    __update_logical_mappings_files(context, deployment_name, args)


def __update_user_mappings_from_file(context, player_mappings_file):
    mappings = {}
    with open(player_mappings_file) as mappings_file:
        mappings = json.load(mappings_file)
    context.config.set_user_mappings(mappings.get("LogicalMappings", {}))

def __update_logical_mappings_files(context, deployment_name, args=None):
    exclusions = __get_mapping_exclusions(context)
    ## AuthenticatedPlayer is a superset of general "Player" and "AuthenticatedPlayer" permissions
    ## The Player mappings file should include both
    player_mappings = __get_mappings(context, deployment_name, exclusions, 'AuthenticatedPlayer', args)

    #update the user-settings.json file if this is the default deployment stack or a explicit deployment stack parameter was received in the CLI args list.
    #If a explicit deployment stack parameter was received then someone is requesting the mappings to be set for to that specific deployment regardless whether it is the default
    if args:
        print("Deployment: ", args.deployment)
    else:
        print("Deployment: ", "None")
    print("Deployment (Default): ", context.config.default_deployment)
    if context.config.default_deployment == deployment_name or args and args.deployment:
        print("Setting the user mappings to", player_mappings)
        context.config.set_user_mappings(player_mappings)

    __update_launcher(context, deployment_name, 'Player', player_mappings, args)
    __update_launcher(context, deployment_name, 'Server', None, args)

def __update_launcher(context, deployment_name, role, mappings=None, args=None):
    if mappings is None:
        mappings = __get_mappings(context, deployment_name, __get_mapping_exclusions(context), role, args)

    kLogicalMappingsObjectName = 'LogicalMappings'
    outData = {}
    outData[kLogicalMappingsObjectName] = mappings
    outData['Protected'] = deployment_name in context.config.get_protected_depolyment_names()

    logicalMappingsFileName = '{}.{}.{}'.format(deployment_name, role.lower(), constant.MAPPING_FILE_SUFFIX)
    logicalMappingsPath = __logical_mapping_file_path(context)
    logicalMappingsPath = os.path.join(logicalMappingsPath, logicalMappingsFileName)

    context.config.save_json(logicalMappingsPath, outData)

def __logical_mapping_file_path(context):
    logicalMappingsPath = context.config.root_directory_path
    logicalMappingsPath = os.path.join(logicalMappingsPath, context.config.game_directory_name)
    logicalMappingsPath = os.path.join(logicalMappingsPath, 'Config')

    return logicalMappingsPath


def __remove_old_mapping_files(context, deployment_name = None):
    if deployment_name is None:
        __remove_all_old_mapping_files(context)
    logicalMappingsPath = __logical_mapping_file_path(context)
    server_mappings_file = os.path.join(logicalMappingsPath, '{}.{}.{}'.format(deployment_name, 'server', constant.MAPPING_FILE_SUFFIX))
    player_mappings_file = os.path.join(logicalMappingsPath, '{}.{}.{}'.format(deployment_name, 'player', constant.MAPPING_FILE_SUFFIX))
    if os.path.exists(player_mappings_file) and context.config.validate_writable(player_mappings_file):
        os.remove(player_mappings_file)
    if os.path.exists(server_mappings_file) and context.config.validate_writable(server_mappings_file):
        os.remove(server_mappings_file)

def __remove_all_old_mapping_files(context):
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


def __get_mapping_exclusions(context):
    exclusions = set(context.config.local_project_settings.get('ExcludedMappings', []))
    exclusions.update(set(context.config.local_project_settings.get('LauncherExcludedMappings', [])))
    return exclusions


def __get_mappings(context, deployment_name, exclusions, role, args=None):
    iam = context.aws.client('iam')
    mappings = {}
    deployment_stack_id = context.config.get_deployment_stack_id(deployment_name)

    region = util.get_region_from_arn(deployment_stack_id)
    account_id = util.get_account_id_from_arn(deployment_stack_id)

    # Assemble and add the iam role ARN to the server mappings
    deployment_access_stack_id = context.config.get_deployment_access_stack_id(deployment_name, True if args is not None and args.is_gui else False)
    server_role_id = context.stack.get_physical_resource_id(deployment_access_stack_id, role, optional=True)
    server_role_arn = iam.get_role(RoleName=server_role_id).get('Role', {}).get('Arn', '')

    context.view.retrieving_mappings(deployment_name, deployment_stack_id, role)

    player_accessible_arns = __get_player_accessible_arns(context, deployment_name, role, args)
    lambda_client = context.aws.client('lambda', region=region)

    resources = context.stack.describe_resources(deployment_stack_id, recursive=True)

    for logical_name, description in resources.iteritems():

        if logical_name in exclusions:
            continue

        physical_resource_id = custom_resource_utils.get_embedded_physical_id(description.get('PhysicalResourceId'))
        if physical_resource_id:
            if __is_user_pool_resource(description):
                mappings[logical_name] = {
                    'PhysicalResourceId': physical_resource_id,
                    'ResourceType': description['ResourceType'],
                    'UserPoolClients': description['UserPoolClients'] # include client id / secret
                }
            else:
                stack_id = description['StackId']
                s3_client = context.aws.client('s3')
                type_definitions = context.resource_types.get_type_definitions_for_stack_id(stack_id, s3_client)

                resource_arn = aws_utils.get_resource_arn(
                    type_definitions=type_definitions,
                    stack_arn=stack_id,
                    resource_type=description['ResourceType'],
                    physical_id=physical_resource_id,
                    optional = True,
                    lambda_client=lambda_client
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
                    'PhysicalResourceId': custom_resource_utils.
                        get_embedded_physical_id(description['PhysicalResourceId']),
                    'ResourceType': description['ResourceType']
                }

    if 'region' not in exclusions:
        mappings['region'] = { 'PhysicalResourceId': region, 'ResourceType': 'Configuration' }

    if 'account_id' not in exclusions:
        mappings['account_id'] = { 'PhysicalResourceId': account_id, 'ResourceType': 'Configuration' }

    if 'server_role_arn' not in exclusions and role is not 'AuthenticatedPlayer':
        mappings['server_role_arn'] = { 'PhysicalResourceId': server_role_arn, 'ResourceType': 'Configuration' }

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
        raise HandledError('The {} resource group template defines a ServiceApi resource but does not provide a ServiceUrl Output value.'.format(resource_group_name))

def __get_player_accessible_arns(context, deployment_name, role, args=None):

    player_accessible_arns = set()

    deployment_access_stack_id = context.config.get_deployment_access_stack_id(deployment_name, True if args is not None and args.is_gui else False)
    player_role_id = context.stack.get_physical_resource_id(deployment_access_stack_id, role, optional=True)

    if player_role_id.startswith("{"): # same check that happens in custom_resource info class
        player_role_id = custom_resource_utils.get_embedded_physical_id(player_role_id).split("/")[-1]
    if player_role_id == None:
        return {}

    iam = context.aws.client('iam')

    res = {}
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
