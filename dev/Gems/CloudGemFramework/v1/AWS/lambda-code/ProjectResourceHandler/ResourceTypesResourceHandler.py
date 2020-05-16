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

import boto3
import copy
import json
import time

from botocore.exceptions import ClientError

from cgf_utils import aws_utils
from cgf_utils import custom_resource_response
from cgf_utils import properties
from cgf_utils import role_utils

from resource_manager_common import constant
from resource_manager_common import resource_type_info
from resource_manager_common import stack_info

s3_client = aws_utils.ClientWrapper(boto3.client("s3"))
iam_client = aws_utils.ClientWrapper(boto3.client("iam"))

_inline_policy_name = "Default"

# Default custom resource lambda timeout in MB
_default_lambda_memory = 128

# Default custom resource lambda timeout in Seconds, can be between 3-900 as per Lambda spec
_default_lambda_timeout = 10  # Seconds

# Schema for _handler_properties_configuration
# Set these values to increase memory and timeout values for a given custom resource Lambda
# These are mostly Lambda configuration properties and should follow existing names and restrictions
_handler_properties_configuration = {
    'MemorySize': properties.Integer(default=_default_lambda_memory),  # MB: Must be a multiple of 64MB as per Lambda spec
    'Timeout': properties.Integer(default=_default_lambda_timeout),  # Seconds: Must be between 3-900 as per Lambda spec
}

# Schema for ArnHandler or FunctionHandlers for core resource types
_handler_schema = {
    'Function': properties.String(""),
    'HandlerFunctionConfiguration': properties.Object(default={}, schema=_handler_properties_configuration),
    'PolicyStatement': properties.ObjectOrListOfObject(default=[], schema={
        'Sid': properties.String(""),
        'Action': properties.StringOrListOfString(),
        'Resource': properties.StringOrListOfString(default=[]),
        'Effect': properties.String(),
        'Condition': properties.Dictionary(default={})
    })
}

# Schema for the CoreResourceTypes custom CloudFormation resources.
# Note: LambdaConfiguration and LambdaTimeout are globally applied to all custom resource Lambdas.  To change Memory and Timeout for a given Lambda
# use a HandlerFunctionConfiguration which overrides the global lambda configs
#
# Note: Need to define expected fields here to avoid "Property is not supported" failures during definition validation
_schema = {
    'LambdaConfiguration': properties.Dictionary(default={}),
    'LambdaTimeout': properties.Integer(default=_default_lambda_timeout),
    'Definitions': properties.Object(default=None, schema={
        '*': properties.Object(default={}, schema={
            'PermissionMetadata': properties.Object(default={}, schema={
                'RestrictActions': properties.StringOrListOfString(default=[]),
                'DefaultRoleMappings': properties.ObjectOrListOfObject(default=[], schema={
                    'AbstractRole': properties.StringOrListOfString(default=[]),
                    'Action': properties.StringOrListOfString(default=[]),
                    'ResourceSuffix': properties.StringOrListOfString(default=[])
                })
            }),
            'ServiceApi': properties.String(""),
            'ArnFormat': properties.String(""),
            'DisplayInfo': properties.Object({}),
            'ArnFunction': properties.Object(default={}, schema=_handler_schema),
            'HandlerFunction': properties.Object(default={}, schema=_handler_schema)
        })
    })
}

# First entry in all arrays below is for the arn lambda, second is for the handler function created for custom resources
# All arrays below need to have have the same number of items to use with zip
_lambda_fields = ["ArnFunction", "HandlerFunction"]
_lambda_tags = [resource_type_info.ARN_LAMBDA_TAG, resource_type_info.CUSTOM_RESOURCE_LAMBDA_TAG]
_lambda_descriptions = ["Fetches the ARN for the %s resource type.", "Handles create/update/delete for the %s resource type."]


def handler(event, context):
    event_type = event['RequestType']
    stack_arn = event['StackId']
    stack_manager = stack_info.StackInfoManager()
    stack = stack_manager.get_stack_info(stack_arn)
    if not stack.is_project_stack:
        raise RuntimeError("Resource Types can only be defined in the project stack.")
    configuration_bucket = stack.project_stack.configuration_bucket
    source_resource_name = event['LogicalResourceId']
    props = properties.load(event, _schema)
    definitions_src = event['ResourceProperties']['Definitions']
    lambda_client = _create_lambda_client(stack_arn)
    created_or_updated_lambdas = {}
    lambda_roles = []

    # Set up tags for all resources created, must be project stack
    # Note: IAM takes an array of [ {'Key':, 'Value':}] format, Lambda take a dict of {string: string} pairs
    iam_tags = [
        {'Key': constant.PROJECT_NAME_TAG, 'Value': stack.stack_name},
        {'Key': constant.STACK_ID_TAG, 'Value': stack_arn}
    ]
    lambda_tags = {constant.PROJECT_NAME_TAG: stack.stack_name, constant.STACK_ID_TAG: stack_arn}

    # Build the file key as "<root directory>/<project stack>/<deployment stack>/<resource_stack>/<resource_name>.json"
    path_components = [x.stack_name for x in stack.ancestry]
    path_components.insert(0, constant.RESOURCE_DEFINITIONS_PATH)
    path_components.append(source_resource_name + ".json")
    resource_file_key = aws_utils.s3_key_join(*path_components)
    path_info = resource_type_info.ResourceTypesPathInfo(resource_file_key)

    # Load information from the JSON file if it exists.
    # (It will exist on a Create event if the resource was previously deleted and recreated.)
    try:
        contents = s3_client.get_object(Bucket=configuration_bucket, Key=resource_file_key)['Body'].read()
        existing_info = json.loads(contents)
        definitions_dictionary = existing_info['Definitions']
        existing_lambdas = existing_info['Lambdas']
        if isinstance(existing_lambdas, dict):
            lambda_dictionary = existing_lambdas
        else:
            # Backwards compatibility
            lambda_dictionary = {}
            existing_lambdas = set([x.split(":")[6] for x in existing_lambdas])  # Convert arn to function name
    except ClientError as e:
        error_code = e.response['Error']['Code']
        if error_code == 'NoSuchKey':
            definitions_dictionary = {}
            existing_lambdas = {}
            lambda_dictionary = {}
        else:
            raise e

    # Process the actual event
    if event_type == 'Delete':
        deleted_entries = set(definitions_dictionary.keys())

    else:
        definitions = props.Definitions
        lambda_config_src = event['ResourceProperties'].get('LambdaConfiguration', None)

        # Create lambdas for fetching the ARN and handling the resource creation/update/deletion
        lambdas_to_create = []

        for resource_type_name in definitions_src.keys():
            type_info = resource_type_info.ResourceTypeInfo(
                stack_arn, source_resource_name, resource_type_name, lambda_dictionary, False,
                definitions_src[resource_type_name])
            function_infos = [type_info.arn_function, type_info.handler_function]

            for function_info, field, tag, description in zip(function_infos, _lambda_fields, _lambda_tags,
                                                              _lambda_descriptions):
                if function_info is None:
                    continue

                function_handler = function_info.get('Function', None)
                if function_handler is None:
                    raise RuntimeError("Definition for '%s' in type '%s' requires a 'Function' field with the handler "
                                       "to execute." % (field, resource_type_name))

                # Create the role for the lambda(s) that will be servicing this resource type
                lambda_function_name = type_info.get_lambda_function_name(tag)
                role_name = role_utils.sanitize_role_name(lambda_function_name)
                role_path = "/%s/%s/" % (type_info.stack_name, type_info.source_resource_name)
                assume_role_policy_document = role_utils.get_assume_role_policy_document_for_service("lambda.amazonaws.com")

                try:
                    res = iam_client.create_role(
                        RoleName=role_name,
                        AssumeRolePolicyDocument=assume_role_policy_document,
                        Path=role_path,
                        Tags=iam_tags)
                    role_arn = res['Role']['Arn']
                except ClientError as e:
                    if e.response["Error"]["Code"] != 'EntityAlreadyExists':
                        raise e
                    res = iam_client.get_role(RoleName=role_name)
                    role_arn = res['Role']['Arn']

                # Copy the base policy for the role and add any permissions that are specified by the type
                role_policy = copy.deepcopy(_create_base_lambda_policy())
                role_policy['Statement'].extend(function_info.get('PolicyStatement', []))
                iam_client.put_role_policy(RoleName=role_name, PolicyName=_inline_policy_name,
                                           PolicyDocument=json.dumps(role_policy))

                # Record this role and the type_info so we can create a lambda for it
                lambda_roles.append(role_name)

                lambda_info = {
                    'role_arn': role_arn,
                    'type_info': type_info,
                    'lambda_function_name': lambda_function_name,
                    'handler': "resource_types." + function_handler,
                    'description': description,
                    'tags': lambda_tags
                }

                # Merge in any lambda specific configs overrides
                if 'HandlerFunctionConfiguration' in function_info:
                    lambda_override = function_info['HandlerFunctionConfiguration']
                    if lambda_override:
                        print("Found LambdaConfiguration override {}".format(lambda_override))
                        lambda_info['lambda_config_overrides'] = lambda_override

                lambdas_to_create.append(lambda_info)

        # We create the lambdas in a separate pass because role-propagation to lambda takes a while, and we don't want
        # to have to delay multiple times for each role/lambda pair
        #
        # TODO: Replace delay (and all other instances of role/lambda creation) with exponential backoff
        time.sleep(role_utils.PROPAGATION_DELAY_SECONDS)

        for info in lambdas_to_create:
            # Create the lambda function
            arn, version = _create_or_update_lambda_function(
                lambda_client=lambda_client,
                timeout=props.LambdaTimeout,
                lambda_config_src=lambda_config_src,
                info=info,
                existing_lambdas=existing_lambdas
            )
            created_or_updated_lambdas[info['lambda_function_name']] = {'arn': arn, 'v': version}

            # Finally add/update a role policy to give least privileges to the Lambdas to log events
            policy_document = _generate_lambda_log_event_policy(arn)
            iam_client.put_role_policy(RoleName=aws_utils.get_role_name_from_role_arn(info['role_arn']),
                                       PolicyDocument=json.dumps(policy_document),
                                       PolicyName='LambdaLoggingEventsPolicy')

        deleted_entries = set(definitions_dictionary.keys()) - set(definitions_src.keys())

    physical_resource_id = "-".join(path_components[1:])
    lambda_dictionary.update(created_or_updated_lambdas)
    definitions_dictionary.update(definitions_src)
    config_info = {
        'StackId': stack_arn,
        'Id': physical_resource_id,
        'Lambdas': lambda_dictionary,
        'Definitions': definitions_dictionary,
        'Deleted': list(deleted_entries)
    }
    data = {
        'ConfigBucket': configuration_bucket,
        'ConfigKey': resource_file_key
    }

    # Copy the resource definitions to the configuration bucket.
    s3_client.put_object(Bucket=configuration_bucket, Key=resource_file_key, Body=json.dumps(config_info, indent=2))
    custom_resource_response.succeed(event, context, data, physical_resource_id)


def _create_lambda_client(stack_arn):
    """Create new lambda client to use. This is to support patching while testing"""
    return aws_utils.ClientWrapper(boto3.client("lambda", aws_utils.get_region_from_stack_arn(stack_arn)))


def _apply_custom_lambda_overrides(lambda_config, info):
    """Ensure that we apply any overrides to the standard lambda config for a custom resource"""
    if 'lambda_config_overrides' in info:
        if 'Timeout' in info['lambda_config_overrides']:
            lambda_config.update({'Timeout': int(info['lambda_config_overrides']['Timeout'])})
        if 'MemorySize' in info['lambda_config_overrides']:
            lambda_config.update({'MemorySize': int(info['lambda_config_overrides']['MemorySize'])})


def _create_or_update_lambda_function(lambda_client, timeout, lambda_config_src, info, existing_lambdas):
    type_info = info['type_info']
    if lambda_config_src is None:
        raise RuntimeError("A Custom::LambdaConfig must be provided as the 'LambdaConfig' parameter for the '%s' "
                           "resource in order to use Arn and Handler functions" % type_info.source_resource_name)

    lambda_name = info['lambda_function_name']
    lambda_config = copy.deepcopy(lambda_config_src)

    if lambda_name in existing_lambdas:
        # Ensure resources are always tagged
        tags = info['tags']
        if len(tags) > 0:
            response = lambda_client.get_function(FunctionName=lambda_name)
            if 'Configuration' in response:
                lambda_client.tag_resource(Resource=response['Configuration']['FunctionArn'], Tags=tags)

        # Just update the function code if it already exists
        src_config = lambda_config['Code']
        src_config['FunctionName'] = lambda_name
        src_config['Publish'] = True
        result = lambda_client.update_function_code(**src_config)

    else:
        # Create the new lambda
        lambda_config.update(
            {
                'FunctionName': lambda_name,
                'Role': info['role_arn'],
                'Handler': info['handler'],
                'Timeout': timeout,
                'Description': info['description'] % type_info.resource_type_name,
                'Publish': True,
                'Tags': info['tags']
            }
        )

        _apply_custom_lambda_overrides(lambda_config, info)
        result = lambda_client.create_function(**lambda_config)

    arn = result.get('FunctionArn', None)
    version = result.get('Version', None)

    if arn is None or version is None:
        raise RuntimeError("Unable to create the lambda for type '{}' in resource '{}'".format(type_info.resource_type_name, type_info.source_resource_name))

    return arn, version


def _create_base_lambda_policy():
    """Generate base secure policy for handling logs

    As the policy is created before Lambdas, do not put anything here that needs to be scoped to the Lambda.
    Instead attach that policy associated with Lambda's execution role after creation, see _generate_lambda_log_event_policy

    Note: CreateLogGroup can not be restricted by resource
    https://docs.aws.amazon.com/IAM/latest/UserGuide/list_amazoncloudwatchlogs.html
    """
    return {
        'Version': "2012-10-17",
        'Statement': [
            {
                'Sid': "WriteLogs",
                'Effect': "Allow",
                'Action': [
                    "logs:CreateLogGroup"
                ],
                'Resource': [
                    "*"
                ]
            }
        ]
    }


def _generate_lambda_log_event_policy(lambda_arn):
    """Generate secure policy for handling logs.

    :param string lambda_arn: The Lambda arn to associate the policy with, to ensure least privileges for logging events
    :return A policy document suitable for attaching via PutRolePolicy

    Note: This policy is attached to the Lambda's execution role after the Lambda is created
    """
    arn_parser = aws_utils.ArnParser(lambda_arn)
    region = arn_parser.region
    aws_account_id = arn_parser.account_id
    lambda_function_name = arn_parser.resource_id

    # Strip out information about version if present <lambda_name>:<version> will form the resource_id
    if lambda_function_name.find(':') != -1:
        lambda_function_name = lambda_function_name.rsplit(':', 1)[0]

    log_group_arn = \
        aws_utils.ArnParser.format_arn_with_resource_type(service="logs", region=region, account_id=aws_account_id, resource_type="log-group",
                                                          resource_id="/aws/lambda/{}".format(lambda_function_name), separator=":")

    log_stream_arn = \
        aws_utils.ArnParser.format_arn_with_resource_type(service="logs", region=region, account_id=aws_account_id, resource_type="log-group",
                                                          resource_id="/aws/lambda/{}:log-stream:*".format(lambda_function_name), separator=":")

    return {
        "Version": "2012-10-17",
        "Statement": [
            {
                "Sid": "WriteLogEvents",
                "Effect": "Allow",
                "Action": [
                    "logs:PutLogEvents",
                    "logs:CreateLogStream"
                ],
                "Resource": [
                    log_group_arn,
                    log_stream_arn
                ]
            }
        ]
    }

