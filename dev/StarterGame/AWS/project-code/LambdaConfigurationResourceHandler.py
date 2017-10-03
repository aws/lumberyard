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
import discovery_utils
import time
import role_utils
from zipfile import ZipFile, ZipInfo
from StringIO import StringIO
from errors import ValidationError
from botocore.exceptions import ClientError
from uuid import uuid4

iam = boto3.client('iam')
s3 = boto3.client('s3')

POLICY_NAME='FunctionAccess'

DEFAULT_POLICY_STATEMENTS = [
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
    logical_role_name = event['LogicalResourceId']
    physical_resource_name = event.get('PhysicalResourceId', None) # None when create request

    if request_type == 'Delete':

        role_utils.delete_role(stack_arn, logical_role_name, POLICY_NAME)

        data = {}

    else:

        policy_metadata_filter = lambda entry: _policy_metadata_filter(entry, props.FunctionName)

        if request_type == 'Create':

            physical_resource_name = discovery_utils.get_stack_name_from_stack_arn(stack_arn) + '-' + event['LogicalResourceId']
    
            assume_role_service = 'lambda.amazonaws.com'
            role_arn = role_utils.create_role(
                stack_arn, 
                logical_role_name, 
                POLICY_NAME, 
                assume_role_service, 
                DEFAULT_POLICY_STATEMENTS, 
                policy_metadata_filter)

        elif request_type == 'Update':

            role_arn = role_utils.update_role(
                stack_arn, 
                logical_role_name, 
                POLICY_NAME, 
                DEFAULT_POLICY_STATEMENTS, 
                policy_metadata_filter)

        else:
            raise ValidationError('Unexpected request type: {}'.format(request_type))

        input_key = '{}/lambda-function-code.zip'.format(props.ConfigurationKey)
        output_key = _inject_settings(props.Settings.__dict__, props.Runtime, props.ConfigurationBucket, input_key, props.FunctionName)

        data = {
            'ConfigurationBucket': props.ConfigurationBucket,
            'ConfigurationKey': output_key,
            'Runtime': props.Runtime,
            'Role': role_arn
        }

    custom_resource_response.succeed(event, context, data, physical_resource_name)


def _policy_metadata_filter(entry, function_name):

        metadata_function_name = entry.get('FunctionName', None)
        
        if not metadata_function_name:
            raise ValidationError('No FunctionName specified.')

        if not isinstance(metadata_function_name, basestring):
            raise ValidationError('Non-string FunctionName specified.')

        return metadata_function_name == function_name


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
    'python2.7': _inject_settings_python,
    'nodejs': _inject_settings_nodejs
}


def _inject_settings(settings, runtime, bucket, input_key, function_name):

    if len(settings) == 0:
        return input_key

    output_key = input_key + '.' + function_name + '.configured'

    injector = _SETTINGS_INJECTORS.get(runtime, None)
    if injector is None:
        raise ValidationError('No setting injector found for runtime {}'.format(runtime))

    res = s3.get_object(Bucket=bucket, Key=input_key)
    zip_content = StringIO(res['Body'].read())
    with ZipFile(zip_content, 'a') as zip_file:
        injector(zip_file, settings)

    res = s3.put_object(Bucket=bucket, Key=output_key, Body=zip_content.getvalue())

    zip_content.close()

    return output_key


