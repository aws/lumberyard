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
s3 = aws_utils.ClientWrapper(boto3.client('s3'), do_not_log_args = ['Body'])

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
    )
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
        # give access to project level ServiceDirectory APIs
        # Other deployment-level APIs are handled in InterfaceDependeny resolver custom resource type
        permitted_arns = _add_services_settings(stack, props.Settings.__dict__, props.Services)
        _add_service_access_policy_to_role(role_arn, permitted_arns)
        # Check if we have a folder just for this function, if not use the default
        input_key = _get_input_key(props)

        output_key = _inject_settings(props.Settings.__dict__, props.Runtime, props.ConfigurationBucket, input_key, props.FunctionName)

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
                'Role': role_arn,
                'Runtime': props.Runtime
            }
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

    if stack.deployment_access:
        pool = stack.deployment_access.resources.get_by_logical_id("PlayerAccessIdentityPool", "Custom::CognitoIdentityPool", optional = True)
        if pool:
            settings["CloudCanvas::IdentityPool"] = pool.physical_id
            print 'Adding setting CloudCanvas::IdentityPool = {}'.format(settings["CloudCanvas::IdentityPool"])
        else:
            print 'Skipping setting CloudCanvas::IdentityPool: PlayerAccessIdentityPool not found.'
    else:
        print 'Skipping setting CloudCanvas::IdentityPool: access stack not found.'

    if stack.project:

        # TODO: remove ServiceLambda and switch CloudGemPlayerAccount over to using service interfaces.
        project_service_lambda = stack.project.resources.get_by_logical_id("ServiceLambda", "AWS::Lambda::Function", optional = True)
        if project_service_lambda:
            settings["CloudCanvas::ServiceLambda"] = project_service_lambda.physical_id
            print 'Adding setting CloudCanvas::ServiceLambda = {}'.format(settings["CloudCanvas::ServiceLambda"])
        else:
            print 'Skipping setting CloudCanvas::ServiceLambda: resource not found.'

    else:
        print 'Skipping setting CloudCanvas::ServiceLambda: project stack not found.'

    if stack.deployment:
        settings["CloudCanvas::DeploymentName"] = stack.deployment.deployment_name
        print 'Adding setting CloudCanvas::DeploymentName = {}'.format(settings["CloudCanvas::DeploymentName"])
    else:
        print 'Skipping setting CloudCanvas::DeploymentName: deployment stack not found.'


def _add_services_settings(stack, settings, services):

    permitted_arns = []

    if not services:
        print 'Not adding service settings because there are none.'
        return permitted_arns

    if not stack.deployment:
        print 'Not adding service settings because the stack is not associated with a deployment.'
        return permitted_arns

    if not stack.project:
        raise RuntimeError(
            'Not adding service settings because there is no project stack.')

    configuration_bucket_name = stack.project.configuration_bucket
    if not configuration_bucket_name:
        raise RuntimeError(
            'Not adding service settings because there is no project configuration bucket.')

    service_directory = ServiceDirectory(configuration_bucket_name)

    services_settings = settings.setdefault('Services', {})
    for service in services:
        _add_service_settings(stack, service_directory,
                              service, services_settings, permitted_arns)

    return permitted_arns


def _add_service_settings(stack, service_directory, service, services_settings, permitted_arns):

    target_gem_name, target_interface_name, target_interface_version = service_interface.parse_interface_id(
        service.InterfaceId)
    if target_gem_name != CLOUD_GEM_FRAMEWORK:
        return
    # try to find the interface from project stack
    interfaces = service_directory.get_interface_services(
        None, service.InterfaceId)

    if len(interfaces) == 0 and not service.Optional:
        raise RuntimeError(
            'There is no service providing {}.'.format(service.InterfaceId))

    if len(interfaces) > 1:
        raise RuntimeError(
            'There are multiple services providing {}.'.format(service.InterfaceId))

    if len(interfaces) > 0:
        interface = interfaces[0]
        services_settings[service.InterfaceId] = interface

        _add_permitted_arns(stack, interface, permitted_arns)


def _add_permitted_arns(stack, interface, permitted_arns):
    ARN_FORMAT = 'arn:aws:execute-api:{region}:{account_id}:{api_id}/{stage_name}/{HTTP_VERB}/{interface_path}/*'

    interface_url_parts = _parse_interface_url(interface['InterfaceUrl'])

    try:
        swagger = json.loads(interface['InterfaceSwagger'])
    except Exception as e:
        raise ValueError('Could not parse interface {} ({}) swagger: {}'.format(
            interface['InterfaceId'], interface['InterfaceUrl'], e.message))

    for swagger_path, path_object in swagger.get('paths', {}).iteritems():
        for operation in path_object.keys():
            if operation in ['get', 'put', 'delete', 'post', 'head', 'options', 'patch', 'trace']:
                arn = ARN_FORMAT.format(
                    region=interface_url_parts.region,
                    account_id=stack.account_id,
                    api_id=interface_url_parts.api_id,
                    stage_name=interface_url_parts.stage_name,
                    HTTP_VERB=operation.upper(),
                    interface_path=interface_url_parts.path
                )
                permitted_arns.append(arn)

InterfaceUrlParts = collections.namedtuple('InterfaceUrlparts', ['api_id', 'region', 'stage_name', 'path'])
INTERFACE_URL_FORMAT = 'https://{api_id}.execute-api.{region}.amazonaws.com/{stage_name}/{path}'

def _parse_interface_url(interface_url):

    slash_parts = interface_url.split('/')
    if len(slash_parts) <= 4:
        raise RuntimeError('Interface url does not have the expected format of {}: {}'.format(INTERFACE_URL_FORMAT, interface_url))
    dot_parts = slash_parts[2].split('.')
    if len(dot_parts) != 5:
        raise RuntimeError('Interface url does not have the expected format of {}: {}'.format(INTERFACE_URL_FORMAT, interface_url))

    api_id = dot_parts[0]
    region = dot_parts[2]
    stage_name = slash_parts[3]
    path = '/'.join(slash_parts[4:])

    return InterfaceUrlParts(
        api_id = api_id,
        region = region,
        stage_name = stage_name,
        path = path
    )


def _add_service_access_policy_to_role(role_arn, permitted_arns):

    role_name = aws_utils.get_role_name_from_role_arn(role_arn)

    if permitted_arns:

        policy_document = copy.deepcopy(SERVICE_ACCESS_POLICY_DOCUMENT)
        policy_document['Statement'][0]['Resource'] = permitted_arns

        iam.put_role_policy(
            RoleName=role_name,
            PolicyName=SERVICE_ACCESS_POLICY_NAME,
            PolicyDocument=json.dumps(policy_document)
        )

    else:

        try:
            iam.delete_role_policy(
                RoleName=role_name,
                PolicyName=SERVICE_ACCESS_POLICY_NAME
            )
        except ClientError as e:
            if e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
                raise e

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
