#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import os
import swagger_processor

import resource_manager.util

from resource_manager.uploader import Uploader
from resource_manager.errors import HandledError


AWS_S3_BUCKET_NAME = 'CloudGemPortal'
BUCKET_ROOT_DIRECTORY_NAME = 'www'

def before_resource_group_updated(hook, resource_group_uploader, **kwargs):
    swagger_path = os.path.join(resource_group_uploader.resource_group.directory_path, 'swagger.json')
    if os.path.isfile(swagger_path):
        resource_group_uploader.context.view.processing_swagger(swagger_path)
        result = swagger_processor.process_swagger_path(swagger_path)
        resource_group_uploader.upload_content('swagger.json', result, 'processed swagger')

def before_project_updated(hook, project_uploader, **kwargs):
    swagger_path = os.path.join(project_uploader.context.config.framework_aws_directory_path, 'swagger.json')
    if os.path.isfile(swagger_path):
        project_uploader.context.view.processing_swagger(swagger_path)
        result = swagger_processor.process_swagger_path(swagger_path)
        project_uploader.upload_content('swagger.json', result, 'processed swagger')
    
def after_project_updated(hook, **kwargs):	
    content_path = os.path.abspath(os.path.join(__file__, '..', '..', BUCKET_ROOT_DIRECTORY_NAME))
    upload_project_content(hook.context, content_path)

def upload_project_content(context, content_path):

    if not os.path.isdir(content_path):        
        raise HandledError('Cloud Gem Portal project content not found at {}.'.format(content_path))
    
    uploader = Uploader(
        context,
        bucket = context.stack.get_physical_resource_id(context.config.project_stack_id, AWS_S3_BUCKET_NAME),    
        key = ''
    )

    uploader.upload_dir(BUCKET_ROOT_DIRECTORY_NAME, content_path)        


# version constants
V_1_0_0 = resource_manager.util.Version('1.0.0')    
V_1_1_0 = resource_manager.util.Version('1.1.0')    
V_1_1_1 = resource_manager.util.Version('1.1.1')


def add_framework_version_update_writable_files(hook, from_version, to_version, writable_file_paths, **kwargs):

    # Repeat this pattern to add more upgrade processing:
    #
    # if from_version < VERSION_CHANGE_WAS_INTRODUCED:
    #     add files that may be written to

    # For now only need to update local-project-settings.json, which will always 
    # be writable when updating the framework version.
    pass


def before_framework_version_updated(hook, from_version, to_version, **kwargs):
    '''Called by the framework before updating the project's framework version.'''

    if from_version < V_1_1_1:
        hook.context.resource_groups.before_update_framework_version_to_1_1_1()

    # Repeat this pattern to add more upgrade processing:
    #
    # if from_version < VERSION_CHANGE_WAS_INTRODUCED:
    #     call an upgrade function (don't put upgrade logic in this function)


def after_framework_version_updated(hook, from_version, to_version, **kwargs):
    '''Called by the framework after updating the project's framework version.'''

    # Repeat this pattern to add more upgrade processing:
    #
    # if from_version < VERSION_CHANGE_WAS_INTRODUCED:
    #     call an upgrade function (don't put upgrade logic in this function)

    # currently don't have anything to do...
    pass

