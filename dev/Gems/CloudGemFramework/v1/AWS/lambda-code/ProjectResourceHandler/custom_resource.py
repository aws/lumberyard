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

from cgf_utils import custom_resource_response
import traceback
import os
import imp
import sys
import json
from cgf_utils import aws_utils
from cgf_utils import json_utils
from resource_manager_common import constant
from resource_manager_common import module_utils
from resource_manager_common import stack_info

from cgf_utils.properties import ValidationError

import boto3

# This is patched by unit tests
PLUGIN_DIRECTORY_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), 'plugin'))

_LOCAL_CUSTOM_RESOURCE_WHITELIST = {"Custom::LambdaConfiguration", "Custom::ResourceTypes"}


def handler(event, context):
    try:

        print 'Dispatching event {} with context {}.'.format(json.dumps(event, cls=json_utils.SafeEncoder), context)

        resource_type = event.get('ResourceType', None)
        if resource_type is None:
            raise RuntimeError('No ResourceType specified.')

        if resource_type in _LOCAL_CUSTOM_RESOURCE_WHITELIST:
            # Old method for supporting custom resource code directly within the ProjectResourceHandler.
            # Should only be used for legacy types in the ProjectResourceHandler.
            module_name = resource_type.replace('Custom::', '') + 'ResourceHandler'

            module = sys.modules.get(module_name, None)

            if module is None:

                # First check for handler module in same directory as this module,
                # if not found, check for module in the resource group provided
                # directories.

                module_file_name = module_name + '.py'
                module_file_path = os.path.join(os.path.dirname(__file__), module_file_name)

                if os.path.isfile(module_file_path):

                    module = module_utils.load_module(module_name, os.path.dirname(module_file_path))

                elif os.path.isdir(PLUGIN_DIRECTORY_PATH):

                    plugin_directory_names = [item for item in os.listdir(PLUGIN_DIRECTORY_PATH) if os.path.isdir(os.path.join(PLUGIN_DIRECTORY_PATH, item))]

                    for plugin_directory_name in plugin_directory_names:
                        module_file_path = os.path.join(PLUGIN_DIRECTORY_PATH, plugin_directory_name, module_file_name)
                        if os.path.isfile(module_file_path):
                            module = module_utils.load_module(module_name, os.path.dirname(module_file_path))
                            break

            if module is not None:
                if not hasattr(module, 'handler'):
                    raise RuntimeError('No handler function found for the {} resource type.'.format(resource_type))

                print 'Using {}'.format(module)

                module.handler(event, context)

        else:
            # New way of instantiating custom resources. Load the dictionary of resource types.
            stack = stack_info.StackInfoManager().get_stack_info(event['StackId'])
            type_definition = stack.resource_definitions.get(resource_type, None)

            if type_definition is None:
                raise RuntimeError('No type definition found for the {} resource type.'.format(resource_type))

            if type_definition.handler_function is None:
                raise RuntimeError('No handler function defined for custom resource type {}.'.format(resource_type))

            lambda_client = aws_utils.ClientWrapper(boto3.client("lambda", stack.region))
            lambda_data = { 'Handler': type_definition.handler_function }
            lambda_data.update(event)
            response = lambda_client.invoke(
                FunctionName=type_definition.get_custom_resource_lambda_function_name(),
                Payload=json.dumps(lambda_data)
            )

            if response['StatusCode'] == 200:
                response_data = json.loads(response['Payload'].read().decode())
                response_success = response_data.get('Success', None)
                if response_success is not None:
                    if response_success:
                        custom_resource_response.succeed(event, context, response_data['Data'],
                                                         response_data['PhysicalResourceId'])
                    else:
                        custom_resource_response.fail(event, context, response_data['Reason'])
                else:
                    raise RuntimeError("Handler lambda for resource type '%s' returned a malformed response: %s" %
                                       (resource_type, response_data))
            else:
                raise RuntimeError("Handler lambda for resource type '%s' failed to execute, returned HTTP status %d" %
                                   (resource_type, response['StatusCode']))

    except ValidationError as e:
        custom_resource_response.fail(event, context, str(e))
    except Exception as e:
        print 'Unexpected error occured when processing event {} with context {}. {}'.format(event, context, traceback.format_exc())
        custom_resource_response.fail(event, context, 'Unexpected {} error occured: {}. Additional details can be found in the CloudWatch log group {} stream {}'.format(
            type(e).__name__,
            e.message,
            context.log_group_name,
            context.log_stream_name))
