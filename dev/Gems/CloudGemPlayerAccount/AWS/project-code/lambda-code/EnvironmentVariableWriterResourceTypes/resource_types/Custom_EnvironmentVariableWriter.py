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

import boto3
from cgf_utils import custom_resource_response
from resource_manager_common import stack_info

def empty_handler(event, context):
    return custom_resource_response.success_response({}, '*')

def handler(event, context):
    ''' Invoked when AWS Lambda service executes. '''

    stack_manager = stack_info.StackInfoManager()
    access_stack_arn = event['StackId']
    access_stack = stack_manager.get_stack_info(access_stack_arn)

    if not access_stack.resources:
        print 'Skipping setting CloudCanvasIdentityPool: access stack not found.'
    else:
        pool = access_stack.resources.get_by_logical_id('PlayerAccessIdentityPool', 'Custom::CognitoIdentityPool', optional = True)
        custom_auth_flow_lambda = __get_resource(access_stack, event['ResourceProperties'].get('GemName', ''), 'CustomAuthFlowLambda', 'AWS::Lambda::Function')

        if not pool:
            print 'Skipping setting CloudCanvasIdentityPool: PlayerAccessIdentityPool not found.'
        elif not custom_auth_flow_lambda:
            print 'Skipping setting CloudCanvasIdentityPool: CustomAuthFlowLambda not found.'
        else:
            print 'Adding setting CloudCanvasIdentityPool = {}'.format(pool.physical_id)

            cloud_canvas_identity_pool_mapping = {'CloudCanvasIdentityPool' : pool.physical_id}
            __add_environment_variables(custom_auth_flow_lambda.physical_id, cloud_canvas_identity_pool_mapping)

    return custom_resource_response.success_response({}, '*')

def __get_resource(stack, gem_name, resource_name, resource_type):
    if not stack.deployment:
        print 'Skipping setting CloudCanvasIdentityPool: deployment stack not found.'
    elif not stack.deployment.resource_groups:
        print 'Skipping setting CloudCanvasIdentityPool: deployment stack resource groups not found.'
    else:
        for resource_group in stack.deployment.resource_groups:
            if resource_group.resource_group_name != gem_name:
                continue

            custom_auth_flow_lambda = resource_group.resources.get_by_logical_id(resource_name, resource_type, optional = True)
            return custom_auth_flow_lambda

    return None

def __add_environment_variables(function_name, environment_variables_mapping):
    lambda_client = boto3.client('lambda')

    function_config = lambda_client.get_function_configuration(
        FunctionName=function_name)
    function_env = function_config.get('Environment', {})
    env_vars = function_env.get('Variables', {})
    env_vars.update(environment_variables_mapping)
    function_env['Variables'] = env_vars

    lambda_client.update_function_configuration(
        FunctionName=function_name, Environment=function_env)