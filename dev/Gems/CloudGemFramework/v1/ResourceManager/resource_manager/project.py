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
# $Revision: #2 $

import fnmatch
import os
import json

import mappings
import deployment
import resource_group
import util
import cognito_pools

from botocore.exceptions import ClientError
from boto3.session import Session

from cgf_utils import aws_utils
from cgf_utils import custom_resource_utils
from cgf_utils import lambda_utils
from errors import HandledError
from util import Args, load_template
from uploader import ProjectUploader
from resource_manager_common import constant
from resource_manager_common import resource_type_info
from copy import deepcopy
import security
import time
import re

def create_stack(context, args):

    '''Implements the lmbr_aws initialze-project command.'''

    # Supported region?
    supported_regions = __get_region_list()
    if args.region not in supported_regions:
        raise HandledError('Region {} is not supported.'.format(args.region))

    # Initialize AWS directory if needed.
    context.config.initialize_aws_directory()
    if(args.files_only):
        return

    # Already initialized?
    if context.config.local_project_settings.project_stack_exists():
        raise HandledError('The project has already been initialized and is using the {} AWS Cloud Formation stack.'.format(context.config.project_stack_id))

    # Project settings writable?
    context.config.validate_writable(context.config.local_project_settings.path)

    # Is it ok to do this?
    pending_resource_status = __get_pending_resource_status(context)
    if not re.match('^[a-z](?:[a-z0-9]*[\-]?[a-z0-9]+)*$', args.stack_name, re.I):
        raise HandledError('Project stack name can only consist of letters, numbers, non-repeating hyphens and must start with a letter: {}'.format(args.stack_name))

    capabilities = context.stack.confirm_stack_operation(
        context.config.get_pending_project_stack_id(), # may be None, which is ok
        'project',
        args,
        pending_resource_status
    )

    # Skip creating the stack if we have already done so.
    if not context.config.get_pending_project_stack_id():

        # Use game directory name as stack name by default.
        if args.stack_name is None:
            args.stack_name = context.config.game_directory_name

        # Does a stack with the name already exist?
        if context.stack.name_exists(args.stack_name, args.region):
            message = 'An AWS Cloud Formation stack with the name {} already exists in region {}. Use the --stack-name option to provide a different name.'.format(args.stack_name, args.region)
            raise HandledError(message)

        # Is the stack name valid?
        util.validate_stack_name(args.stack_name)

        # Create stack using the boostrapping template.
        context.stack.create_using_template(
            args.stack_name,
            bootstrap_template,
            args.region,
            created_callback=lambda id: context.config.set_pending_project_stack_id(id),
            capabilities = capabilities,
            timeoutinminutes = 30
        )

    # Create initial project settings.
    context.config.init_project_settings()

    # Temporarily set the config's project_stack_id property to the pending stack
    # id so the project uploader can find it later.
    # context.config.project_stack_id = context.config.get_pending_project_stack_id()

    # Do the initial update...
    __update_project_stack(context, pending_resource_status, capabilities, args)


def update_framework_version(context, args):

    current_framework_version = context.config.framework_version
    if context.gem.framework_gem.version == current_framework_version:
        raise HandledError('The framework version used by the project is already {}, the same version as the enabled CloudGemFramework gem.'.format(current_framework_version))

    # Project settings writable?

    writable_file_paths = set( [ context.config.local_project_settings.path ] )
    context.hooks.call_module_handlers('resource-manager-code/update.py', 'add_framework_version_update_writable_files',
        kwargs={
            'from_version': current_framework_version,
            'to_version': context.gem.framework_gem.version,
            'writable_file_paths': writable_file_paths
        }
    )

    if not util.validate_writable_list(context, writable_file_paths):
        return

    context.view.updating_framework_version(current_framework_version, context.gem.framework_gem.version)
    context.config.set_pending_framework_version(context.gem.framework_gem.version)

    context.hooks.call_module_handlers('resource-manager-code/update.py', 'before_framework_version_updated',
        kwargs={
            'from_version': current_framework_version,
            'to_version': context.gem.framework_gem.version
        }
    )

    if context.config.project_initialized:

        # Is it ok to do this?
        pending_resource_status = __get_pending_resource_status(context)
        capabilities = context.stack.confirm_stack_operation(
            context.config.project_stack_id,
            'project stack',
            args,
            pending_resource_status
        )

        __update_project_stack(context, pending_resource_status, capabilities, args)

    context.hooks.call_module_handlers('resource-manager-code/update.py', 'after_framework_version_updated',
        kwargs={
            'from_version': current_framework_version,
            'to_version': context.gem.framework_gem.version
        }
    )

    context.config.save_pending_framework_version()


def update_stack(context, args):
    # Has the project been initialized?
    if not context.config.project_initialized:

        # If the create worked but the first update failed, the gui and the user may try
        # to recover by doing an update instead of retrying the create. If that is the case,
        # verify that the settings file can be updated once the project is initialized. Also,
        # temporarily set the config's project_stack_id property to the pending stack
        # id so the project uploader can find it later.
        pending_project_stack_id = context.config.get_pending_project_stack_id()
        if pending_project_stack_id:
            context.config.validate_writable(context.config.local_project_settings_path)
            context.config.project_stack_id = pending_project_stack_id
        else:
            raise HandledError('The project has not been initialized.')

    # Assume role explicitly because we don't read any project config, and
    # that is what usually triggers it (project config must be read before
    # assuming the role).
    context.config.assume_role()

    # Is it ok to do this?
    pending_resource_status = __get_pending_resource_status(context)
    capabilities = context.stack.confirm_stack_operation(
        context.config.project_stack_id,
        'project stack',
        args,
        pending_resource_status
    )

    # Do the update...
    __update_project_stack(context, pending_resource_status, capabilities, args)


def __update_project_stack(context, pending_resource_status, capabilities, args):
    # Upload the project template and code directory.

    project_uploader = ProjectUploader(context)

    context.view.processing_template('project')

    project_template_url = project_uploader.upload_content(
        constant.PROJECT_TEMPLATE_FILENAME,
        json.dumps(context.config.project_template_aggregator.effective_template, indent=4, sort_keys=True),
        'processed project template'
    )

    __zip_individual_lambda_code_folders(context, project_uploader)

    if os.path.exists(context.config.join_aws_directory_path(constant.COGNITO_POOLS_FILENAME)):
        project_uploader.upload_file(constant.COGNITO_POOLS_FILENAME, context.config.join_aws_directory_path(
            constant.COGNITO_POOLS_FILENAME))
    # Deprecated in 1.9. TODO: remove
    # Execute all the uploader pre hooks before the resources are updated
    project_uploader.execute_uploader_pre_hooks()

    kwargs = {
            'project_uploader': project_uploader,
            'args': args
        }
    context.hooks.call_module_handlers('resource-manager-code/update.py', 'before_project_updated',
        kwargs=kwargs
    )

    # wait a bit for S3 to help insure that templates can be read by cloud formation
    time.sleep(constant.STACK_UPDATE_DELAY_TIME)

    # Update the stack
    parameters = __get_parameters(context, project_uploader)

    context.stack.update(
        context.config.project_stack_id,
        project_template_url,
        parameters=parameters,
        pending_resource_status=pending_resource_status,
        capabilities=capabilities
    )

    # wait a bit for S3 to help insure that templates can be read by cloud formation
    time.sleep(constant.STACK_UPDATE_DELAY_TIME)

    # Project is fully initialized only after the first successful update
    # Project resource intialization is based on having a ProjectStackId in your local-project-settings.json
    # Post hooks could be dependant on the availability of project_resources
    # So we save the pending stack id which is used during Project stack creation
    # Then we reinitialize the config.__project_resources to make the project_resources available to hooks even during a project stack creation
    context.config.save_pending_project_stack_id()
    if args.record_cognito_pools:
        __record_cognito_pools(context)
    # Deprecated in 1.9. TODO: remove
    # Now all the stack resources should be available to the hooks
    project_uploader.execute_uploader_post_hooks()

    context.hooks.call_module_handlers('resource-manager-code/update.py', 'after_project_updated',
        kwargs=kwargs
    )


def __record_cognito_pools(context):
    pools = {
        "Project": {}
    }

    for resource_name, definition in context.config.project_resources.iteritems():
        if definition["ResourceType"] in ["Custom::CognitoIdentityPool", "Custom::CognitoUserPool"]:
                pools["Project"][resource_name] = {
                    "PhysicalResourceId": custom_resource_utils.get_embedded_physical_id(definition['PhysicalResourceId']),
                    "Type": definition["ResourceType"]
                }
    cognito_pools.write_to_project_file(context, pools)



def __zip_individual_lambda_code_folders(context, uploader):
    resources = context.config.project_template_aggregator.effective_template.get("Resources", {})
    uploaded_folders = []

    # Iterating over LambdaConfiguration resources first, as the lambdas without them are special cases.
    # Just future proofing against further specialization on ProjectResourceHandler code
    for name, description in  resources.iteritems():
        if not description["Type"] == "Custom::LambdaConfiguration":
            continue
        function_name = description["Properties"]["FunctionName"]
        uploaded_folders.append(function_name)
        aggregated_directories = None
        source_gem_name = description.get("Metadata", {}).get("CloudGemFramework", {}).get("Source", None)
        uploader.upload_lambda_function_code(function_name, function_runtime=description["Properties"]["Runtime"], source_gem=context.gem.get_by_name(
            source_gem_name))

    # There's some untagling needed to allow uploading ProjectResourceHandler without a LambdaConfiguration
    # We should generally avoid adding any more functions like this to the project stack and eventually do away with it.
    for name, description in  resources.iteritems():
        aggregated_directories = None
        if not description["Type"] == "AWS::Lambda::Function":
            continue
        if name in uploaded_folders:
            print "We already uploaded this with the Corresponding LambdaConfiguration resource"
            continue

        if name == "ProjectResourceHandler":
            aggregated_directories = __get_plugin_project_code_paths(context)
        uploader.upload_lambda_function_code(
            name, function_runtime=description["Properties"]["Runtime"], aggregated_directories=aggregated_directories)


def __get_plugin_project_code_paths(context):

    plugin_project_code_paths = {}

    for group in context.resource_groups.values():
        resource_group_path = group.directory_path
        resource_group_project_code_path = os.path.join(resource_group_path, 'project-code', 'plugin')
        if os.path.isdir(resource_group_project_code_path):
            plugin_project_code_paths[os.path.join('plugin', group.name)] = resource_group_project_code_path

    for gem in context.gem.enabled_gems:
        gem_project_code_path = os.path.join(gem.aws_directory_path, 'project-code', 'plugin')
        if os.path.isdir(gem_project_code_path):
            plugin_project_code_paths[os.path.join('plugin', gem.name)] = gem_project_code_path

    return plugin_project_code_paths


def __get_parameters(context, uploader):
    return {
        'ConfigurationKey': uploader.key if uploader else None
    }


def __get_pending_resource_status(context, deleting=False):

    stack_id = context.config.project_stack_id
    if not stack_id:
        stack_id = context.config.get_pending_project_stack_id()

    if deleting:
        template = {}
        parameters = {}
    else:
        template = context.config.project_template_aggregator.effective_template
        parameters = __get_parameters(context, uploader=None)

    lambda_function_content_paths = []

    resources = context.config.project_template_aggregator.effective_template.get("Resources", {})
    for name, description in  resources.iteritems():
        if not description["Type"] == "AWS::Lambda::Function":
            continue

        function_runtime = lambda_utils.get_function_runtime(name, description, resources)

        code_path, imported_paths, multi_imports = ProjectUploader.get_lambda_function_code_paths(
            context, name, function_runtime)

        lambda_function_content_paths.append(code_path)
        lambda_function_content_paths.extend(imported_paths)

        if name == 'ProjectResourceHandler':
            lambda_function_content_paths.extend(__get_plugin_project_code_paths(context).values())

    # TODO: need to support swagger.json IN the lambda directory.
    service_api_content_paths = [ os.path.join(context.config.framework_aws_directory_path, 'swagger.json') ]

    # TODO: get_pending_resource_status's new_content_paths parameter needs to support
    # a per-resource mapping instead of an per-type mapping. As is, a change in any lambda
    # directory makes all lambdas look like they need to be updated.
    return context.stack.get_pending_resource_status(
        stack_id,
        new_template = template,
        new_parameter_values = parameters,
        new_content_paths = {
            'AWS::Lambda::Function': lambda_function_content_paths,
            'Custom::ServiceApi': service_api_content_paths
        }
    )


def delete_stack(context, args):

    if context.config.project_stack_id is None:
        raise HandledError("Project stack does not exist.")

    if context.config.deployment_names:
        raise HandledError('The project has {} deployment stack(s): {}. You must delete these stacks before you can delete the project stack.'.format(
            len(context.config.deployment_names),
            ', '.join(deployment_name for deployment_name in context.config.deployment_names)))

    if context.stack.id_exists(context.config.project_stack_id):

        retained_bucket_names = ["Configuration", "Logs"]
        retained_bucket_ids = [context.stack.get_physical_resource_id(context.config.project_stack_id, name, optional=True, expected_type='AWS::S3::Bucket') for name in retained_bucket_names]

        pending_resource_status = __get_pending_resource_status(context, deleting=True)
        context.stack.confirm_stack_operation(
            context.config.project_stack_id,
            'project stack',
            args,
            pending_resource_status
        )

        context.stack.delete(context.config.project_stack_id, pending_resource_status = pending_resource_status)

        __delete_custom_resource_lambdas(context, args)

        for retained_bucket_name, retained_bucket_id in zip(retained_bucket_names, retained_bucket_ids):
            if retained_bucket_id:

                s3 = context.aws.client('s3')

                # Check to see if the bucket still exists, old versions of project-template.json
                # don't have DeletionPolicy="Retain" on this bucket.
                try:
                    s3.head_bucket(Bucket=retained_bucket_id)
                    bucket_still_exists = True
                except:
                    bucket_still_exists = False

                if bucket_still_exists:
                    stack_name = util.get_stack_name_from_arn(context.config.project_stack_id)
                    util.delete_bucket_contents(context, stack_name, retained_bucket_name, retained_bucket_id)
                    context.view.deleting_bucket(retained_bucket_id)
                    s3.delete_bucket(Bucket=retained_bucket_id)

    else:

        context.view.clearing_project_stack_id(context.config.project_stack_id)

    context.config.clear_project_stack_id()


def __delete_custom_resource_lambdas(context, args):
    context.view.deleting_custom_resource_lambdas()
    stack_id = context.config.project_stack_id
    project_name = util.get_stack_name_from_arn(stack_id)
    region = util.get_region_from_arn(stack_id)
    lambda_client = context.aws.client('lambda', region=region)
    iam_client = context.aws.client('iam')
    delete_functions = []
    delete_roles = []
    prefixes = ["{}-{}-".format(project_name, prefix) for prefix in resource_type_info.LAMBDA_TAGS]

    # Iterate through all lambda functions and generate a list that begin with any of the prefixes associated with
    # custom resource handlers
    for response in lambda_client.get_paginator('list_functions').paginate():
        for entry in response['Functions']:
            function_name = entry['FunctionName']
            if any(function_name.startswith(prefix) for prefix in prefixes):
                delete_functions.append(function_name)
                delete_roles.append(aws_utils.get_role_name_from_role_arn(entry['Role']))

    # Delete the functions and roles related to custom resource handlers
    for function_name, role_name in zip(delete_functions, delete_roles):
        lambda_client.delete_function(FunctionName=function_name)
        iam_client.delete_role_policy(RoleName=role_name, PolicyName="Default")
        iam_client.delete_role(RoleName=role_name)

    context.view.deleting_lambdas_completed(len(delete_functions))


def clean_custom_resource_handlers(context, args):
    if context.config.project_stack_id is None:
        raise HandledError("Project stack does not exist.")

    context.view.deleting_custom_resource_lambdas()
    lambda_client = context.aws.client('lambda', region=util.get_region_from_arn(context.config.project_stack_id))
    project_info = context.stack_info.manager.get_stack_info(context.config.project_stack_id)
    resource_types_used_versions = {}
    delete_count = 0

    def add_resource_versions(stack_info):
        for resource_info in stack_info.resources:
            if resource_info.type.startswith("Custom::"):
                info = custom_resource_utils.get_custom_resource_info(resource_info.physical_id)
                if info.create_version:
                    resource_types_used_versions.setdefault(resource_info.type, set()).add(info.create_version)
                    metadata_version = resource_info.get_cloud_canvas_metadata(
                        custom_resource_utils.METADATA_VERSION_TAG)
                    if metadata_version:
                        resource_types_used_versions[resource_info.type].add(metadata_version)

    # Add the resources from the project stack, the deployment stacks, and all the resource groups
    add_resource_versions(project_info)

    for deployment_info in project_info.deployments:
        add_resource_versions(deployment_info)

        for resource_group_info in deployment_info.resource_groups:
            add_resource_versions(resource_group_info)

    # Iterate over the custom resource types
    for resource_type_name, resource_type_info in project_info.resource_definitions.iteritems():
        if resource_type_info.handler_function:
            # Obtain a list of all versions of the function
            lambda_function_name = resource_type_info.get_custom_resource_lambda_function_name()
            versions = []

            for result in aws_utils.paginate(
                    lambda_client.list_versions_by_function, {'FunctionName': lambda_function_name}):
                versions.extend([entry['Version'] for entry in result['Versions']])

            # Walk through all versions older than the current version, and delete them if they are not in use
            assert(len(versions) >= 2)
            assert(versions[0] == "$LATEST")
            assert(int(versions[-1]) == max([int(x) for x in versions[1:]]))  # Last entry should be greatest version
            in_use_versions = resource_types_used_versions.get(resource_type_name, set())

            for version in versions[1:-1]:
                if version not in in_use_versions:
                    context.view.deleting_lambda(lambda_function_name, version)
                    lambda_client.delete_function(FunctionName=lambda_function_name, Qualifier=version)
                    delete_count += 1

    context.view.deleting_lambdas_completed(delete_count)


def deprecated_list_resources(context, args):
    if args.stack_id:
        resource_descriptions = context.stack.describe_resources(stack_id)
        context.view.deprecated_resource_list(args.stack_id, resource_descriptions)
    elif args.deployment and args.resource_group:
        resource_group.list_resource_group_resources(context, args)
    elif args.deployment:
        deployment.list_deployment_resources(context, args)
    else:
        list_project_resources(context, args)


def list_project_resources(context, args):

    # Assume role explicitly because we don't read any project config, and
    # that is what usually triggers it (project config must be read before
    # assuming the role).
    context.config.assume_role()

    context.view.project_resource_list(
        context.config.project_stack_id or context.config.get_pending_project_stack_id(),
        __get_pending_resource_status(context)
    )


def describe(context, args):

    '''Provides information about the project. Used by the GUI.'''

    # Initialize AWS directory if needed.
    context.config.initialize_aws_directory()

    description = {
        'ProjectInitialized': context.config.project_initialized,
        'ProjectInitializing': not context.config.project_initialized and context.config.get_pending_project_stack_id() is not None,
        'HasAWSDirectoryContent': context.config.has_aws_directory_content,
        'ProjectSettingsFilePath' : context.config.local_project_settings.path,
        'UserSettingsFilePath': context.config.user_settings_path,
        'ProjectTemplateFilePath': context.config.project_template_aggregator.extension_file_path,
        'DeploymentTemplateFilePath': context.config.deployment_template_aggregator.extension_file_path,
        'DeploymentAccessTemplateFilePath': context.config.deployment_access_template_aggregator.extension_file_path,
        'AWSCredentialsFilePath': context.aws.get_credentials_file_path(),
        'ProjectCodeDirectoryPath': context.config.project_lambda_code_path,
        'DefaultAWSProfile': context.config.user_default_profile,
        'GuiRefreshFilePath': context.config.gui_refresh_file_path,
        'ProjectUsesAWS': True if context.resource_groups.keys() else False,
        'GemsFilePath': context.gem.get_gems_file_path()
    }

    context.view.project_description(description)


def describe_stack(context, args):
    if context.config.project_stack_id is not None:
        stack_description = context.stack.describe_stack(context.config.project_stack_id)
    elif context.config.get_pending_project_stack_id() is not None:
        stack_description = context.stack.describe_stack(context.config.get_pending_project_stack_id())
    else:
        stack_description = {
            'StackStatus': '',
            'PendingAction': context.stack.PENDING_CREATE,
            'PendingReason': 'The project stack has not been created in AWS.'
        }
    context.view.project_stack_description(stack_description)


def get_regions(context, args):
    supported_regions = __get_region_list()

    context.view.supported_region_list(supported_regions)

def __get_region_list():
    s = Session()
    core_services = ['cognito-identity', 'cognito-idp', 'dynamodb','kinesis', 'lambda', 's3', 'sns', 'sqs', 'sts']
    supported_regions = s.get_available_regions('cloudformation')
    for core_service in core_services:
        supported_regions = list(set(supported_regions) & set(s.get_available_regions(core_service)))

    return supported_regions

def __find_existing_files(src_path, dst_path):

    list = []

    for root, dirs, files in os.walk(src_path):

        relative_path = os.path.relpath(root, src_path)
        if relative_path == '.':
            dst_directory = dst_path
        else:
            dst_directory = os.path.join(dst_path, relative_path)

        if os.path.exists(dst_directory):
            for file in files:
                src_file = os.path.join(root, file)
                dst_file = os.path.join(dst_directory, file)
                if os.path.exists(dst_file):
                    list.append(dst_file)

    return list


def __filter_writeable_files(input_list):
        filtered_list = []
        for file_path in input_list:
            if not os.access(file_path, os.W_OK):
                filtered_list.append(file_path)
        return filtered_list


def create_extension_template(context, args):

    if args.project:
        context.config.project_template_aggregator.save_extension_template()

    if args.deployment:
        context.config.deployment_template_aggregator.save_extension_template()

    if args.deployment_access:
        context.config.deployment_access_template_aggregator.save_extension_template()


bootstrap_template = '''{
    "AWSTemplateFormatVersion": "2010-09-09",

    "Parameters": {

        "CloudCanvasStack": {
            "Type": "String",
            "Description": "Identifies this stack as a Lumberyard Cloud Canvas managed stack.",
            "Default": "Project"
        }

    },

    "Resources": {

        "Configuration": {
            "Type": "AWS::S3::Bucket",
            "Properties": {
                "VersioningConfiguration": {
                    "Status": "Enabled"
                },
                "LifecycleConfiguration": {
                    "Rules": [
                        {
                            "Id": "DeleteOldVersions",
                            "NoncurrentVersionExpirationInDays": "2",
                            "Status": "Enabled"
                        },
                        {
                            "Id": "DeleteUploads",
                            "Prefix": "uploads",
                            "ExpirationInDays": 2,
                            "Status": "Enabled"
                        }
                    ]
                }
            }
        }

    }

}'''
