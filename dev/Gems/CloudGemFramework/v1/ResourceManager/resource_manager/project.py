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

import fnmatch
import os
import json
import datetime

import mappings
import deployment
import resource_group
import util
from botocore.exceptions import ClientError
from boto3.session import Session

from errors import HandledError
from util import Args, load_template
from uploader import ProjectUploader
import constant
from copy import deepcopy
import security
import time

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
    if context.config.project_stack_id is not None:
        raise HandledError('The project has already been initialized and is using the {} AWS Cloud Formation stack.'.format(context.config.project_stack_id))

    # Project settings writable?
    context.config.validate_writable(context.config.local_project_settings.path)

    # Is it ok to do this?
    pending_resource_status = __get_pending_resource_status(context)
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
            raise HandledError('An AWS Cloud Formation stack with the name {} already exists in region {}. Use the --stack-name option to provide a different name.'.format(args.stack_name, args.region))

        # Is the stack name valid?
        util.validate_stack_name(args.stack_name)

        # Create stack using the boostrapping template.
        context.stack.create_using_template(
            args.stack_name, 
            bootstrap_template, 
            args.region,
            created_callback=lambda id: context.config.set_pending_project_stack_id(id),
            capabilities = capabilities
        )

    # Create initial project settings.
    context.config.init_project_settings()

    # Temporarily set the config's project_stack_id property to the pending stack
    # id so the project uploader can find it later.
    context.config.project_stack_id = context.config.get_pending_project_stack_id()

    # Do the initial update...
    __update_project_stack(context, pending_resource_status, capabilities)


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

        __update_project_stack(context, pending_resource_status, capabilities)

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
    __update_project_stack(context, pending_resource_status, capabilities)


def __update_project_stack(context, pending_resource_status, capabilities):

    # Upload the project template and code directory.

    project_uploader = ProjectUploader(context)

    context.view.processing_template('project')

    project_template_url = project_uploader.upload_content(
        constant.PROJECT_TEMPLATE_FILENAME,
        json.dumps(context.config.project_template_aggregator.effective_template, indent=4, sort_keys=True),
        'processed project template'
    )
   
    __zip_individual_lambda_code_folders(context, project_uploader)

    # Deprecated in 1.9. TODO: remove
    # Execute all the uploader pre hooks before the resources are updated
    project_uploader.execute_uploader_pre_hooks()

    context.hooks.call_module_handlers('resource-manager-code/update.py', 'before_project_updated', 
        kwargs={
            'project_uploader': project_uploader
        }
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

    # Deprecated in 1.9. TODO: remove
    # Now all the stack resources should be available to the hooks
    project_uploader.execute_uploader_post_hooks()

    context.hooks.call_module_handlers('resource-manager-code/update.py', 'after_project_updated', 
        kwargs={
            'project_uploader': project_uploader
        }
    )

    # Project is fully initialized only after the first successful update
    context.config.save_pending_project_stack_id()


def __zip_individual_lambda_code_folders(context, uploader):

    resources = context.config.project_template_aggregator.effective_template.get("Resources", {})
    for name, description in  resources.iteritems():

        if not description["Type"] == "AWS::Lambda::Function":
            continue

        if name == 'ProjectResourceHandler':
            aggregated_directories = __get_plugin_project_code_paths(context)
        else:
            aggregated_directories = None

        uploader.zip_and_upload_lambda_function_code(name, aggregated_directories = aggregated_directories)


def __get_plugin_project_code_paths(context):

    plugin_project_code_paths = {}

    for group in context.resource_groups.values():
        resource_group_path = group.directory_path
        resource_group_project_code_path = os.path.join(resource_group_path, 'project-code')
        if os.path.isdir(resource_group_project_code_path):
            plugin_project_code_paths[os.path.join('plugin', group.name)] = resource_group_project_code_path

    for gem in context.gem.enabled_gems:
        gem_project_code_path = os.path.join(gem.aws_directory_path, 'project-code')
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

        code_path, imported_paths = ProjectUploader.get_lambda_function_code_paths(context, name)

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

        logs_bucket_id = context.stack.get_physical_resource_id(context.config.project_stack_id, 'Logs', optional=True, expected_type='AWS::S3::Bucket')

        pending_resource_status = __get_pending_resource_status(context, deleting=True)
        context.stack.confirm_stack_operation(
            context.config.project_stack_id,
            'project stack',
            args,
            pending_resource_status
        )

        context.stack.delete(context.config.project_stack_id, pending_resource_status = pending_resource_status)

        if logs_bucket_id:

            s3 = context.aws.client('s3')

            # Check to see if the bucket still exists, old versions of project-template.json
            # don't have DeletionPolicy="Retain" on this bucket.
            try:
                s3.head_bucket(Bucket=logs_bucket_id)
                bucket_still_exists = True
            except:
                bucket_still_exists = False

            if bucket_still_exists:
                stack_name = util.get_stack_name_from_arn(context.config.project_stack_id)
                util.delete_bucket_contents(context, stack_name, "Logs", logs_bucket_id)
                context.view.deleting_bucket(logs_bucket_id)
                s3.delete_bucket(Bucket=logs_bucket_id)

    else:

        context.view.clearing_project_stack_id(context.config.project_stack_id)

    context.config.clear_project_stack_id()


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


def get_function_log(context, args):

    # Assume role explicitly because we don't read any project config, and
    # that is what usually triggers it (project config must be read before 
    # assuming the role).
    context.config.assume_role()

    project_stack_id = context.config.project_stack_id
    if not project_stack_id: 
        project_stack_id = context.config.get_pending_project_stack_id()

    if not project_stack_id:
        raise HandledError('A project stack must be created first.')

    if args.deployment and args.resource_group:
        target_stack_id = context.config.get_resource_group_stack_id(args.deployment, args.resource_group)
    elif args.deployment or args.resource_group:
        raise HandledError('Both the --deployment option and --resource-group must be provided if either is provided.')
    else:
        target_stack_id = project_stack_id

    function_id = context.stack.get_physical_resource_id(target_stack_id, args.function)

    log_group_name = '/aws/lambda/{}'.format(function_id)

    region = util.get_region_from_arn(target_stack_id)
    logs = context.aws.client('logs', region=region)

    if args.log_stream_name:
        limit = 50
    else:
        limit = 1

    log_stream_name = None
    try:
        res = logs.describe_log_streams(logGroupName=log_group_name, orderBy='LastEventTime', descending=True, limit=limit)
    except ClientError as e:
        if e.response['Error']['Code'] == 'ResourceNotFoundException':
            raise HandledError('No logs found.')
        raise e

    for log_stream in res['logStreams']:
        # partial log stream name matches are ok
        if not args.log_stream_name or args.log_stream_name in log_stream['logStreamName']:
            log_stream_name = log_stream['logStreamName']
            break

    if not log_stream_name:
        if args.log_stream_name:
            raise HandledError('No log stream name with {} found in the first {} log streams.'.format(args.log_stream_name, limit))
        else:
            raise HandledError('No log stream was found.')

    res = logs.get_log_events(logGroupName=log_group_name, logStreamName=log_stream_name, startFromHead=True)
    while res['events']:
        for event in res['events']:
            time_stamp = datetime.datetime.fromtimestamp(event['timestamp']/1000.0).strftime("%Y-%m-%d %H:%M:%S")
            message = event['message'][:-1]
            context.view.log_event(time_stamp, message)
        nextForwardToken = res.get('nextForwardToken', None)
        if not nextForwardToken: break
        res = logs.get_log_events(logGroupName=log_group_name, logStreamName=log_stream_name, startFromHead=True, nextToken=nextForwardToken)

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
