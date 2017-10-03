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

import properties
import custom_resource_response
import boto3
import json
import time
import role_utils
from resource_manager_common import aws_utils
import re
from resource_manager_common import stack_info
from zipfile import ZipFile, ZipInfo
from StringIO import StringIO
from botocore.exceptions import ClientError
from uuid import uuid4

iam = boto3.client('iam')
s3 = boto3.client('s3')

POLICY_NAME='FunctionAccess'

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

def handler(event, context):
    
    props = properties.load(event, {
        'ConfigurationBucket': properties.String(),
        'ConfigurationKey': properties.String(),
        'FunctionName': properties.String(),
        'Settings': properties.Object( default={}, 
            schema={
                '*': properties.String()
            }),
        'Runtime': properties.String()
        })

    request_type = event['RequestType']
    stack_arn = event['StackId']
    logical_role_name = props.FunctionName
    
    id_data = aws_utils.get_data_from_custom_physical_resource_id(event.get('PhysicalResourceId', None))

    if request_type == 'Delete':

        role_utils.delete_access_control_role(
            id_data, 
            logical_role_name)

        response_data = {}

    else:

        if request_type == 'Create':

            project_service_lambda_arn = _get_project_service_lambda_arn(stack_arn)

            assume_role_service = 'lambda.amazonaws.com'
            role_arn = role_utils.create_access_control_role(
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

        _add_built_in_settings(props.Settings.__dict__, stack_arn)

        # Check if we have a folder just for this function, if not use the default
        input_key = _get_input_key(props)

        output_key = _inject_settings(props.Settings.__dict__, props.Runtime, props.ConfigurationBucket, input_key, props.FunctionName)

        response_data = {
            'ConfigurationBucket': props.ConfigurationBucket,
            'ConfigurationKey': output_key,
            'Runtime': props.Runtime,
            'Role': role_arn,
            'RoleName': role_utils.get_access_control_role_name(stack_arn, logical_role_name)
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

def _add_built_in_settings(settings, stack_arn):
    stack = stack_info.get_stack_info(stack_arn)
    if stack.stack_type != stack.STACK_TYPE_RESOURCE_GROUP:
        return

    access_stack = stack.deployment.deployment_access
    if access_stack:
        pool = access_stack.resources.get_by_logical_id("PlayerAccessIdentityPool", "Custom::CognitoIdentityPool", True)
        if pool:
            settings["CloudCanvas::IdentityPool"] = pool.physical_id
            print 'Adding setting CloudCanvas::IdentityPool = {}'.format(settings["CloudCanvas::IdentityPool"])
        else:
            print 'Skipping setting CloudCanvas::IdentityPool: PlayerAccessIdentityPool not found.'
    else:
        print 'Skipping setting CloudCanvas::IdentityPool: access stack not found.'

    project_service_lambda = stack.deployment.project.resources.get_by_logical_id("ServiceLambda", "AWS::Lambda::Function", True)
    if project_service_lambda:
        settings["CloudCanvas::ServiceLambda"] = project_service_lambda.physical_id
        print 'Adding setting CloudCanvas::ServiceLambda = {}'.format(settings["CloudCanvas::ServiceLambda"])
    else:
        print 'Skipping setting CloudCanvas::ServiceLambda: resource not found.'

    settings["CloudCanvas::DeploymentName"] = stack.deployment.deployment_name

def _get_project_service_lambda_arn(stack_arn):
    stack = stack_info.get_stack_info(stack_arn)

    if stack.stack_type == stack.STACK_TYPE_RESOURCE_GROUP:
        project_service_lambda = stack.deployment.project.resources.get_by_logical_id("ServiceLambda", "AWS::Lambda::Function", True)
        if project_service_lambda:
            return project_service_lambda.resource_arn
    return None

def _inject_settings_python(zip_file, settings):

    first = True
    content = 'settings = {'
    for k, v in settings.iteritems():
        if not first:
            content += ','
        content += '\n    \'{}\': \'{}\''.format(k, v)
        first = False
    content += '\n}'

    print 'inserting settings', content

    info = ZipInfo('CloudCanvas/settings.py')
    info.external_attr = 0777 << 16L # give full access to included file

    zip_file.writestr(info, content)


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

    output_key = input_key + '.' + function_name + '.configured'

    injector = _get_settings_injector(runtime)

    res = s3.get_object(Bucket=bucket, Key=input_key)
    zip_content = StringIO(res['Body'].read())
    with ZipFile(zip_content, 'a') as zip_file:
        injector(zip_file, settings)

    res = s3.put_object(Bucket=bucket, Key=output_key, Body=zip_content.getvalue())

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

