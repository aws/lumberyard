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

# Python
import collections
import copy
import json
import re
import time
from StringIO import StringIO
from uuid import uuid4
from zipfile import ZipFile, ZipInfo

# Boto3
import boto3
import botocore

from botocore.exceptions import ClientError

# ServiceDirectory
from cgf_service_directory import ServiceDirectory

# ResourceManagerCommon
from resource_manager_common import stack_info, service_interface

# Utils
from cgf_utils import aws_utils
from cgf_utils import custom_resource_response
from cgf_utils import properties
from cgf_utils import role_utils

CLOUD_GEM_FRAMEWORK = 'CloudGemFramework'
iam = aws_utils.ClientWrapper(boto3.client('iam'))
cfg = botocore.config.Config(read_timeout=70, connect_timeout=70)
s3 = aws_utils.ClientWrapper(boto3.client('s3', config=cfg), do_not_log_args = ['Body'])

def get_default_policy(project_service_lambda_arn):
    policy = {
        "Version": "2012-10-17",
        "Id": "Default Lambda Execution Permissions",
        "Statement": [
            {
                "Sid": "WriteLogs",
                "Effect": "Allow",
                "Action": [
                    "logs:CreateLogGroup",
                    "logs:CreateLogStream",
                    "logs:PutLogEvents"
                ],
                "Resource": "arn:aws:logs:*:*:*"
            },
            {
                "Sid": "RunInVPC",
                "Effect": "Allow",
                "Action": [
                    "ec2:CreateNetworkInterface",
                    "ec2:DescribeNetworkInterfaces",
                    "ec2:DeleteNetworkInterface"
                ],
                "Resource": "*"
            }
        ]
    }
    if project_service_lambda_arn:
        policy['Statement'].append({
            "Sid": "InvokeProjectServiceLambda",
            "Effect": "Allow",
            "Action": [
                "lambda:InvokeFunction"
            ],
            "Resource": project_service_lambda_arn
        })
    return json.dumps(policy)


PROPERTIES_SCHEMA = {
    'ConfigurationBucket': properties.String(),
    'ConfigurationKey': properties.String(),
    'FunctionName': properties.String(),
    'Settings': properties.Object( default={},
        schema={
            '*': properties.String()
        }
    ),
    'Runtime': properties.String(),
    'Services': properties.ObjectOrListOfObject( default=[],
        schema={
            'InterfaceId': properties.String(),
            'Optional': properties.Boolean(default = False)
        }
    ),
    'IgnoreAppendingSettingsToZip': properties.Boolean(default = False)
}


def handler(event, context):

    props = properties.load(event, PROPERTIES_SCHEMA)

    request_type = event['RequestType']
    stack_arn = event['StackId']
    logical_role_name = props.FunctionName
    stack_manager = stack_info.StackInfoManager()

    id_data = aws_utils.get_data_from_custom_physical_resource_id(event.get('PhysicalResourceId', None))

    if request_type == 'Delete':

        role_utils.delete_access_control_role(
            id_data,
            logical_role_name)

        response_data = {}

    else:

        stack = stack_manager.get_stack_info(stack_arn)

        if request_type == 'Create':

            project_service_lambda_arn = _get_project_service_lambda_arn(stack)
            assume_role_service = 'lambda.amazonaws.com'
            role_arn = role_utils.create_access_control_role(
                stack_manager,
                id_data,
                stack_arn,
                logical_role_name,
                assume_role_service,
                default_policy = get_default_policy(project_service_lambda_arn))

        elif request_type == 'Update':

            role_arn = role_utils.get_access_control_role_arn(
                id_data,
                logical_role_name)

        else:
            raise RuntimeError('Unexpected request type: {}'.format(request_type))
        _add_built_in_settings(props.Settings.__dict__, stack)
        # Check if we have a folder just for this function, if not use the default
        output_key = input_key = _get_input_key(props)
        if not props.IgnoreAppendingSettingsToZip:
            output_key = _inject_settings(props.Settings.__dict__, props.Runtime, props.ConfigurationBucket, input_key, props.FunctionName)

        cc_settings = copy.deepcopy(props.Settings.__dict__)
        # Remove "Services" from settings because they get injected into the python code package during _inject_settings
        # TODO: move handling of project-level service interfaces to the same code as cross-gemm interfaces
        if "Services" in cc_settings:
            del cc_settings["Services"]
        response_data = {
            'ConfigurationBucket': props.ConfigurationBucket,
            'ConfigurationKey': output_key,
            'Runtime': props.Runtime,
            'Role': role_arn,
            'RoleName': role_utils.get_access_control_role_name(stack_arn, logical_role_name),
            'ComposedLambdaConfiguration': {
                'Code': {
                    'S3Bucket': props.ConfigurationBucket,
                    'S3Key': output_key
                },
                "Environment": {
                    "Variables": cc_settings
                },
                'Role': role_arn,
                'Runtime': props.Runtime
            },
            "CCSettings": cc_settings
        }

    physical_resource_id = aws_utils.construct_custom_physical_resource_id_with_data(stack_arn, event['LogicalResourceId'], id_data)
    custom_resource_response.succeed(event, context, response_data, physical_resource_id)


def _get_input_key(props):
    input_key = '{}/{}-lambda-code.zip'.format(props.ConfigurationKey, props.FunctionName)
    config_bucket = boto3.resource('s3').Bucket(props.ConfigurationBucket)
    objs = list(config_bucket.objects.filter(Prefix=input_key))
    if len(objs) == 0:
        print("There is no unique code folder for {}, using default".format(props.FunctionName))
        input_key = '{}/lambda-function-code.zip'.format(props.ConfigurationKey)
    return input_key


def _add_built_in_settings(settings, stack):
    if stack.project:

        # TODO: remove ServiceLambda and switch CloudGemPlayerAccount over to using service interfaces.
        project_service_lambda = stack.project.resources.get_by_logical_id("ServiceLambda", "AWS::Lambda::Function", optional = True)
        if project_service_lambda:
            settings["CloudCanvasServiceLambda"] = project_service_lambda.physical_id
            print 'Adding setting CloudCanvasServiceLambda = {}'.format(settings["CloudCanvasServiceLambda"])
        else:
            print 'Skipping setting CloudCanvasServiceLambda: resource not found.'

    else:
        print 'Skipping setting CloudCanvasServiceLambda: project stack not found.'

    if stack.deployment:
        settings["CloudCanvasDeploymentName"] = stack.deployment.deployment_name
        print 'Adding setting CloudCanvasDeploymentName = {}'.format(settings["CloudCanvasDeploymentName"])
    else:
        print 'Skipping setting CloudCanvasDeploymentName: deployment stack not found.'

def _get_project_service_lambda_arn(stack):
    if stack.project:
        project_service_lambda = stack.project.resources.get_by_logical_id("ServiceLambda", "AWS::Lambda::Function", optional = True)
        if project_service_lambda:
            return  project_service_lambda.resource_arn
    return None


SERVICE_ACCESS_POLICY_NAME = 'ProjectServiceAccess'

SERVICE_ACCESS_POLICY_DOCUMENT = {
    "Version": "2012-10-17",
    "Statement": [
        {
           "Sid": "InvokeServiceApi",
            "Effect": "Allow",
            "Action": "execute-api:Invoke",
            "Resource": []
        }
    ]
}


def _inject_settings_python(zip_file, settings):

    content = json.dumps(settings, indent = 4, sort_keys = True)

    print 'inserting settings', content

    info = ZipInfo('cgf_lambda_settings/settings.json')
    info.external_attr = 0777 << 16L # give full access to included file
    print 'writing the settings', settings
    zip_file.writestr(info, content)
    print 'completed the write of the settings to the zip file'


def _inject_settings_nodejs(zip_file, settings):

    first = True
    content = 'module.exports = {'
    for k, v in settings.iteritems():
        if not first:
            content += ','
        content += '\n    "{}": "{}"'.format(k, v)
        first = False
    content += '\n};'

    print 'inserting settings', content

    info = ZipInfo('CloudCanvas/settings.js')
    info.external_attr = 0777 << 16L # give full access to included file

    zip_file.writestr(info, content)


_SETTINGS_INJECTORS = {
    'python': _inject_settings_python,
    'nodejs': _inject_settings_nodejs
}


def _inject_settings(settings, runtime, bucket, input_key, function_name):

    if len(settings) == 0:
        return input_key

    if not "Services" in settings:
        print 'No services found in settings, skipping settings injection to code zip in favor of using environment vars'
        return input_key

    service_settings = {}
    service_settings["Services"] = settings.get("Services", {})
    settings = service_settings

    output_key = input_key + '.' + function_name + '.configured'

    injector = _get_settings_injector(runtime)

    print "Downloading the S3 file {}/{} to inject the settings property.".format(bucket, input_key)
    res = s3.get_object(Bucket=bucket, Key=input_key)
    zip_content = StringIO(res['Body'].read())
    zip_file = ZipFile(zip_content, mode='a')
    injector(zip_file, settings)
    zip_file.close()
    print "Uploading the S3 file {}/{} to S3 file {}/{}.".format(bucket, input_key, bucket, output_key)
    res = s3.put_object(Bucket=bucket, Key=output_key, Body=zip_content.getvalue())
    print "Setting injection complete for {}".format(output_key)
    zip_content.close()

    return output_key



def _get_settings_injector(runtime):
    injector = _SETTINGS_INJECTORS.get(runtime, None)
    if injector is None:
        match = re.search("\d", runtime)
        if match:
            versionless_runtime = runtime[:match.start()]
            injector = _SETTINGS_INJECTORS.get(versionless_runtime, None)
        if injector is None:
            raise RuntimeError('No setting injector found for Labmda runtime {}'.format(runtime))
    return injector
