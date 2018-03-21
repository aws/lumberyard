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
# $Revision: #12 $

# python
import datetime
import urllib2

from zipfile import ZipFile
from StringIO import StringIO

from errors import HandledError

# resource_manager
import uploader
import util

def upload_lambda_code(context, args):

    if args.project:

        project_uploader = uploader.ProjectUploader(context)
        __upload_lambda_code(context, context.config.project_stack_id, project_uploader, args.function, args.keep)

    else:

        # Use default deployment if necessary
        if args.deployment is None:
            if context.config.default_deployment is None:
                raise HandledError('No deployment was specified and there is no default deployment configured.')
            args.deployment = context.config.default_deployment

        # The deployment doesn't exist
        if not context.config.deployment_stack_exists(args.deployment):
            raise HandledError('There is no {} deployment stack.'.format(args.deployment))

        # Upload the Lambda function code
        resource_group_stack_id = context.config.get_resource_group_stack_id(args.deployment, args.resource_group)
        project_uploader = uploader.ProjectUploader(context)
        deployment_uploader = project_uploader.get_deployment_uploader(args.deployment)
        resource_group_uploader = deployment_uploader.get_resource_group_uploader(args.resource_group)
        __upload_lambda_code(context, resource_group_stack_id, resource_group_uploader, args.function, args.keep)


def __upload_lambda_code(context, stack_id, uploader, function_name, keep):

    # Create a client representing the lambda resource
    resource_region = util.get_region_from_arn(stack_id)
    client = context.aws.client('lambda', region = resource_region)

    #Get the resources description in the stack
    resources = context.stack.describe_resources(stack_id)

    lambda_function_descriptions = []
    if function_name:
        description = resources.get(function_name, None)
        if description is not None and description['ResourceType'] == 'AWS::Lambda::Function':
            lambda_function_descriptions.append(description)
        else:
            raise HandledError('Lambda function {} does not exist.'.format(function_name))
    else:
        #If the function name isn't given, find the descriptions for all the Lambda functions
        for logical_name, description in resources.iteritems():
            if description['ResourceType'] == 'AWS::Lambda::Function':
                lambda_function_descriptions.append(description)

    for lambda_function_description in lambda_function_descriptions:
    
        # get settings content
        settings_path, settings_content = __get_settings_content(context, client, lambda_function_description)
        aggregated_content = {}
        if settings_path:
            aggregated_content[settings_path] = settings_content

        # zip and send it to s3 in preparation for lambdas
        function_name = lambda_function_description['LogicalResourceId']
        key = uploader.zip_and_upload_lambda_function_code(function_name, aggregated_content = aggregated_content, keep = keep)

        # update the lambda function
        client.update_function_code(FunctionName = lambda_function_description['PhysicalResourceId'], S3Bucket = uploader.bucket, S3Key = key)


def __get_settings_content(context, client, lambda_function_description):

    settings_name = None
    settings_content = None

    context.view.downloading(lambda_function_description['LogicalResourceId'] + ' Lambda Function code to retrieve current settings')

    res = client.get_function(FunctionName=lambda_function_description['PhysicalResourceId'])
    location = res.get('Code', {}).get('Location', None)
    if location:

        zip_content = urllib2.urlopen(location)
        zip_file = ZipFile(StringIO(zip_content.read()), 'r')

        for name in zip_file.namelist():
            if name in ['CloudCanvas/settings.py', 'CloudCanvas/settings.js', 'cgf_lambda_settings/settings.json']:
                settings_name = name
                settings_content = zip_file.open(settings_name, 'r').read()
                break

    return settings_name, settings_content


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

