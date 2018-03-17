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

_lambda_base_policy = {
    'Version': "2012-10-17",
    'Statement': [
        {
            'Sid': "WriteLogs",
            'Effect': "Allow",
            'Action': [
                "logs:CreateLogGroup",
                "logs:CreateLogStream",
                "logs:PutLogEvents"
            ],
            'Resource': "arn:aws:logs:*:*:*"
        }
    ]
}

_handler_schema = {
    'Function': properties.String(""),
    'PolicyStatement': properties.ObjectOrListOfObject(default=[], schema={
        'Sid': properties.String(""),
        'Action': properties.StringOrListOfString(),
        'Resource': properties.StringOrListOfString(default=[]),
        'Effect': properties.String()
    })
}

_schema = {
    'LambdaConfiguration': properties.Dictionary(default={}),
    'LambdaTimeout': properties.Integer(default=10),
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
    lambda_client = aws_utils.ClientWrapper(boto3.client("lambda", aws_utils.get_region_from_stack_arn(stack_arn)))
    lambda_arns = []
    lambda_roles = []

    # Build the file key as "<root directory>/<project stack>/<deployment stack>/<resource_stack>/<resource_name>.json"
    path_components = [x.stack_name for x in stack.ancestry]
    path_components.insert(0, constant.RESOURCE_DEFINITIONS_PATH)
    path_components.append(source_resource_name + ".json")
    resource_file_key = aws_utils.s3_key_join(*path_components)
    path_info = resource_type_info.ResourceTypesPathInfo(resource_file_key)

    # Load information from the JSON file if it exists
    if event_type != 'Create':
        contents = s3_client.get_object(Bucket=configuration_bucket, Key=resource_file_key)['Body'].read()
        existing_info = json.loads(contents)
    else:
        existing_info = None

    # Process the actual event
    if event_type == 'Delete':
        _delete_resources(existing_info['Lambdas'], existing_info['Roles'], lambda_client)
        custom_resource_response.succeed(event, context, {}, existing_info['Id'])

    else:
        existing_roles = set()
        existing_lambdas = set()

        if event_type == 'Update':
            existing_roles = set([arn.split(":")[-1] for arn in existing_info['Roles']])
            existing_lambdas = set([arn.split(":")[-1] for arn in existing_info['Lambdas']])

        definitions = props.Definitions
        lambda_config_src = event['ResourceProperties'].get('LambdaConfiguration', None)

        # Create lambdas for fetching the ARN and handling the resource creation/update/deletion
        lambdas_to_create = []

        for resource_type_name in definitions_src.keys():
            type_info = resource_type_info.ResourceTypeInfo(stack_arn, source_resource_name, resource_type_name,
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
                assume_role_policy_document = role_utils.get_assume_role_policy_document_for_service(
                    "lambda.amazonaws.com")

                try:
                    res = iam_client.create_role(
                        RoleName=role_name,
                        AssumeRolePolicyDocument=assume_role_policy_document,
                        Path=role_path)
                    role_arn = res['Role']['Arn']
                except ClientError as e:
                    if e.response["Error"]["Code"] != 'EntityAlreadyExists':
                        raise e
                    existing_roles.discard(role_name)
                    res = iam_client.get_role(RoleName=role_name)
                    role_arn = res['Role']['Arn']

                # Copy the base policy for the role and add any permissions that are specified by the type
                role_policy = copy.deepcopy(_lambda_base_policy)
                role_policy['Statement'].extend(function_info.get('PolicyStatement', []))
                iam_client.put_role_policy(RoleName=role_name, PolicyName=_inline_policy_name,
                                           PolicyDocument=json.dumps(role_policy))

                # Record this role and the type_info so we can create a lambda for it
                lambda_roles.append(role_name)
                lambdas_to_create.append({
                    'role_arn': role_arn,
                    'type_info': type_info,
                    'lambda_function_name': lambda_function_name,
                    'handler': "resource_types." + function_handler,
                    'description': description
                })

        # We create the lambdas in a separate pass because role-propagation to lambda takes a while, and we don't want
        # to have to delay multiple times for each role/lambda pair
        #
        # TODO: Replace delay (and all other instances of role/lambda creation) with exponential backoff
        time.sleep(role_utils.PROPAGATION_DELAY_SECONDS)

        for info in lambdas_to_create:
            # Create the lambda function
            arn = _create_or_update_lambda_function(
                lambda_client=lambda_client,
                timeout=props.LambdaTimeout,
                lambda_config_src=lambda_config_src,
                info=info,
                existing_lambdas=existing_lambdas
            )
            lambda_arns.append(arn)

        # For Update operations, delete any lambdas and roles that previously existed and now no longer do.
        _delete_resources(existing_lambdas, existing_roles, lambda_client)

    physical_resource_id = "-".join(path_components[1:])
    config_info = {
        'StackId': stack_arn,
        'Id': physical_resource_id,
        'Lambdas': lambda_arns,
        'Roles': lambda_roles,
        'Definitions': definitions_src
    }
    data = {
        'ConfigBucket': configuration_bucket,
        'ConfigKey': resource_file_key
    }

    # Copy the resource definitions to the configuration bucket.
    s3_client.put_object(Bucket=configuration_bucket, Key=resource_file_key, Body=json.dumps(config_info))
    custom_resource_response.succeed(event, context, data, physical_resource_id)


def _create_or_update_lambda_function(lambda_client, timeout, lambda_config_src, info, existing_lambdas):
    type_info = info['type_info']
    if lambda_config_src is None:
        raise RuntimeError("A Custom::LambdaConfig must be provided as the 'LambdaConfig' parameter for the '%s' "
                           "resource in order to use Arn and Handler functions" % type_info.source_resource_name)

    lambda_name = info['lambda_function_name']
    lambda_config = copy.deepcopy(lambda_config_src)

    if lambda_name in existing_lambdas:
        # Just update the function code if it already exists
        existing_lambdas.remove(lambda_name)
        src_config = lambda_config['Code']
        src_config['FunctionName'] = lambda_name
        result = lambda_client.update_function_code(**src_config)
        arn = result.get('FunctionArn', None)

    else:
        # Create the new lambda
        lambda_config.update(
            {
                'FunctionName': lambda_name,
                'Role': info['role_arn'],
                'Handler': info['handler'],
                'Timeout': timeout,
                'Description': info['description'] % type_info.resource_type_name
            }
        )
        result = lambda_client.create_function(**lambda_config)
        arn = result.get('FunctionArn', None)

    if arn is None:
        raise RuntimeError("Unable to create the lambda for type '%s' in resource '%s'" % (
            type_info.resource_type_name, type_info.source_resource_name))

    return arn


def _delete_resources(lambdas, roles, lambda_client):
    for lambda_id in lambdas:
        lambda_client.delete_function(FunctionName=lambda_id)

    for role_id in roles:
        iam_client.delete_role_policy(RoleName=role_id, PolicyName=_inline_policy_name)
        iam_client.delete_role(RoleName=role_id)
