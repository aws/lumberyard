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

from cgf_utils import custom_resource_utils
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

_UPDATE_CHANGED_PHYSICAL_ID_WARNING = "Warning: resource \"{}\" has updated physical resource ID from \"{}\" to " \
    "\"{}\". This is forbidden by the CloudFormation specification for custom resources and may result in " \
    "unspecified behavior."

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

            request_type = event['RequestType']

            if type_definition.deleted and request_type == "Create":
                raise RuntimeError('Attempting to Create a new resource of deleted type {}.'.format(resource_type))

            create_version = type_definition.handler_function_version
            logical_id = event['LogicalResourceId']
            embedded_physical_id = None

            lambda_client = aws_utils.ClientWrapper(boto3.client("lambda", stack.region))
            cf_client = aws_utils.ClientWrapper(boto3.client("cloudformation", stack.region))

            if request_type != "Create":
                physical_id = event['PhysicalResourceId']
                embedded_physical_id = physical_id
                try:
                    existing_resource_info = json.loads(physical_id)
                    embedded_physical_id = existing_resource_info['id']
                    create_version = existing_resource_info['v']
                except (ValueError, TypeError, KeyError):
                    # Backwards compatibility with resources created prior to versioning support
                    create_version = None

            run_version = create_version

            # Check the metadata on the resource to see if we're coercing to a different version
            resource_info = cf_client.describe_stack_resource(StackName=event['StackId'], LogicalResourceId=logical_id)
            metadata = aws_utils.get_cloud_canvas_metadata(resource_info['StackResourceDetail'],
                                                           custom_resource_utils.METADATA_VERSION_TAG)
            if metadata:
                run_version = metadata
                if request_type == "Create":
                    create_version = metadata

            # Configure our invocation, and invoke the handler lambda
            lambda_data = {'Handler': type_definition.handler_function}
            lambda_data.update(event)
            invoke_params = {
                'FunctionName': type_definition.get_custom_resource_lambda_function_name(),
                'Payload': json.dumps(lambda_data)
            }

            if run_version:
                invoke_params['Qualifier'] = run_version

            response = lambda_client.invoke(**invoke_params)

            if response['StatusCode'] == 200:
                response_data = json.loads(response['Payload'].read().decode())
                response_success = response_data.get('Success', None)

                if response_success is not None:
                    if response_success:
                        if create_version:
                            if request_type == "Update" and response_data['PhysicalResourceId'] != embedded_physical_id:
                                # Physical ID changed during an update, which is *technically* illegal according to the
                                # docs, but we allow it because CloudFormation doesn't act to prevent it.
                                print(_UPDATE_CHANGED_PHYSICAL_ID_WARNING.format(
                                    logical_id, embedded_physical_id,
                                    response_data['PhysicalResourceId']))

                            out_resource_id = json.dumps({
                                'id': response_data['PhysicalResourceId'],
                                'v': create_version
                            })
                        else:
                            # Backwards compatibility with resources created prior to versioning support
                            out_resource_id = response_data['PhysicalResourceId']

                        custom_resource_response.succeed(event, context, response_data['Data'], out_resource_id)

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
