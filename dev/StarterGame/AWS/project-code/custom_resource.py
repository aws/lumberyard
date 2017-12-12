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

import custom_resource_response
import traceback
import os
import imp
import sys

from properties import ValidationError

# This is patched by unit tests
PLUGIN_DIRECTORY_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), 'plugin'))

def handler(event, context):
    try:
        
        resource_type = event.get('ResourceType', None)
        if resource_type is None:
            raise RuntimeError('No ResourceType specified.')

        module_name = resource_type.replace('Custom::', '') + 'ResourceHandler'
        
        module = sys.modules.get(module_name, None)

        if module is None:

            # First check for handler module in same directory as this module,
            # if not found, check for module in the resource group provided
            # directories.

            module_file_name = module_name + '.py'
            module_file_path = os.path.join(os.path.dirname(__file__), module_file_name)
        
            if os.path.isfile(module_file_path):

                module = __load_module(module_name, os.path.dirname(module_file_path))

            elif os.path.isdir(PLUGIN_DIRECTORY_PATH):

                plugin_directory_names = [item for item in os.listdir(PLUGIN_DIRECTORY_PATH) if os.path.isdir(os.path.join(PLUGIN_DIRECTORY_PATH, item))]

                for plugin_directory_name in plugin_directory_names:
                    module_file_path = os.path.join(PLUGIN_DIRECTORY_PATH, plugin_directory_name, module_file_name)
                    if os.path.isfile(module_file_path):
                        module = __load_module(module_name, os.path.dirname(module_file_path))
                        break

        if module is None:
            raise RuntimeError('No handler module found for the {} resource type.'.format(resource_type))

        if not hasattr(module, 'handler'):
            raise RuntimeError('No handler function found for the {} resource type.'.format(resource_type))

        print 'Using {}'.format(module)

        module.handler(event, context)

    except ValidationError as e:
        custom_resource_response.fail(event, context, str(e))
    except Exception as e:
        print 'Unexpected error occured when processing event {} with context {}. {}'.format(event, context, traceback.format_exc())
        custom_resource_response.fail(event, context, 'Unexpected error occured. Details can be found in the CloudWatch log group {} stream {}'.format(
            context.log_group_name,
            context.log_stream_name))


def __load_module(name, path):

    imp.acquire_lock()
    try:

        print 'Loading module {} from {}.'.format(name, path)

        sys.path.append(path)
        try:

            fp, pathname, description = imp.find_module(name, [ path ])

            try:
                module = imp.load_module(name, fp, pathname, description)
                return module
            finally:
                if fp:
                    fp.close()

        finally:
            sys.path.remove(path)

    finally:
        imp.release_lock()