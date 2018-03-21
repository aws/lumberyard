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



import json
import boto3
import collections
import copy

from cgf_utils import custom_resource_response
from cgf_utils import properties
from resource_manager_common import stack_info
from cgf_utils import role_utils
from cgf_utils import patch
from cgf_utils import aws_utils
from cgf_utils import json_utils
from botocore.exceptions import ClientError

from cgf_service_directory import ServiceDirectory

lambda_client = boto3.client('lambda')
iam_client = boto3.client('iam')


SERVICE_DIRECTORY_PREFIX = "cloudcanvas_service_"

def handler(event, context):
    request_type = event['RequestType']
    stack_arn = event['StackId']
    physical_resource_id = aws_utils.get_stack_name_from_stack_arn(stack_arn) + '-' + event['LogicalResourceId']
    data = {}
    stack_manager = stack_info.StackInfoManager()
    stack = stack_manager.get_stack_info(stack_arn)

    if request_type == 'Delete':
        _clear_interface_refs(stack)
        return custom_resource_response.success_response(data, physical_resource_id)

    if not stack.is_deployment_stack:
        raise RuntimeError("InterfaceDependecyResolver can only be stood up on a deployment stack")

    resource_groups = stack.resource_groups

    configuration_bucket_name = stack.project.configuration_bucket
    if not configuration_bucket_name:
        raise RuntimeError('Not adding service settings because there is no project configuration bucket.')
    service_directory = ServiceDirectory(configuration_bucket_name)


    interface_deps = event["ResourceProperties"].get("InterfaceDependencies", {})
    _clear_interface_refs(stack)
    for gem, interface_list in interface_deps.iteritems():
        for interface in interface_list:
            print "getting url for interface {} from gem {} to use in {}:{}".format(interface["id"], interface["gem"], gem, interface["function"])
            interfaces = service_directory.get_interface_services(stack.deployment_name, interface["id"])
            if len(interfaces) > 0:
                _add_url_to_lambda(interfaces[0], gem, interface["function"], stack)
            else:
                print "Failed to lookup interface {}".format(len(interfaces))

    return custom_resource_response.success_response(data, physical_resource_id)


def _add_url_to_lambda(interface, resource_group_name, lambda_name, deployment):
    # lambda_function is a stack_info.ResourceInfo
    lambda_function = _get_lambda(resource_group_name, lambda_name, deployment)
    if lambda_function == None:
        return
    function_config = lambda_client.get_function_configuration(FunctionName=lambda_function.physical_id)

    # add URL as an environment variable
    existing_env = function_config.get("Environment", {})
    env_vars = existing_env.get("Variables", {})
    env_vars['{}{}'.format(SERVICE_DIRECTORY_PREFIX, interface['InterfaceId'])] = interface['InterfaceUrl']
    existing_env["Variables"] = env_vars
    response = lambda_client.update_function_configuration(FunctionName=lambda_function.physical_id, Environment=existing_env)

    # add permissions for calling that API
    permitted_arns = _get_permitted_arns(_get_resource_group(resource_group_name, deployment), interface)
    _add_service_access_policy_to_role(function_config["Role"], permitted_arns)


ARN_FORMAT = 'arn:aws:execute-api:{region}:{account_id}:{api_id}/{stage_name}/{HTTP_VERB}/{interface_path}/*'
def _get_permitted_arns(stack, interface):
    permitted_arns = []
    interface_url_parts = _parse_interface_url(interface['InterfaceUrl'])
    try:
        swagger = json.loads(interface['InterfaceSwagger'])
    except Exception as e:
        raise ValueError('Could not parse interface {} ({}) swagger: {}'.format(interface['InterfaceId'], interface['InterfaceUrl'], e.message))

    for swagger_path, path_object in swagger.get('paths', {}).iteritems():
        for operation in path_object.keys():
            if operation in ['get', 'put', 'delete', 'post', 'head', 'options', 'patch', 'trace']:
                arn = ARN_FORMAT.format(
                    region = interface_url_parts.region,
                    account_id = stack.account_id,
                    api_id = interface_url_parts.api_id,
                    stage_name = interface_url_parts.stage_name,
                    HTTP_VERB = operation.upper(),
                    interface_path = interface_url_parts.path
                )
                permitted_arns.append(arn)
    return permitted_arns


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


SERVICE_ACCESS_POLICY_NAME = 'ServiceAccess'

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

def _add_service_access_policy_to_role(role_arn, permitted_arns):

    role_name = aws_utils.get_role_name_from_role_arn(role_arn)

    if permitted_arns:

        policy_document = copy.deepcopy(SERVICE_ACCESS_POLICY_DOCUMENT)
        policy_document['Statement'][0]['Resource'] = permitted_arns

        iam_client.put_role_policy(
            RoleName=role_name,
            PolicyName=SERVICE_ACCESS_POLICY_NAME,
            PolicyDocument=json.dumps(policy_document)
        )

    else:

        try:
            iam_client.delete_role_policy(
                RoleName=role_name,
                PolicyName=SERVICE_ACCESS_POLICY_NAME
            )
        except ClientError as e:
            if e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied"]:
                raise e


def _get_lambda(resource_group_name, lambda_name, deployment):
    all_rgs = deployment.resource_groups
    resource_group_info = _get_resource_group(resource_group_name, deployment)
    for resource in resource_group_info.resources:
        if resource.type == "AWS::Lambda::Function":
            if resource.logical_id == lambda_name:
                return resource
    return None


def _get_resource_group(resource_group_name, deployment):
    for resource in deployment.resources:
        if resource.type == 'AWS::CloudFormation::Stack':
            stack_id = resource.physical_id
            if stack_id is not None and resource_group_name == resource.logical_id:
                return stack_info.ResourceGroupInfo(
                    deployment.stack_manager,
                    stack_id, 
                    resource_group_name=resource.logical_id,
                    session=deployment.session, 
                    deployment_info=deployment)
    return None


def _clear_interface_refs(deployment):
    # for each resource group
    for resource in [r for r in deployment.resources if r.type == 'AWS::CloudFormation::Stack']:
        stack_id = resource.physical_id
        resource_group = stack_info.ResourceGroupInfo(
            deployment.stack_manager,
            stack_id,
            resource_group_name=resource.logical_id,
            session=deployment.session,
            deployment_info=deployment)

        for rg_lambda in [l for l in resource_group.resources if l.type == 'AWS::Lambda::Function']:
            try:
                function_config = lambda_client.get_function_configuration(FunctionName=rg_lambda.physical_id)
                existing_env = function_config.get("Environment", {})
                env_vars = existing_env.get("Variables", {})

                to_remove = [var for var in env_vars if SERVICE_DIRECTORY_PREFIX in var]

                for rem in to_remove:
                    del env_vars[rem]
                existing_env["Variables"] = env_vars
                response = lambda_client.update_function_configuration(FunctionName=rg_lambda.physical_id, Environment=existing_env)
                # Passing an empty list removes the policy.
                _add_service_access_policy_to_role(function_config["Role"], [])
            except ClientError as e:
                if e.response["Error"]["Code"] not in ["NoSuchEntity", "AccessDenied", "ResourceNotFoundException"]:
                    raise e
