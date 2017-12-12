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

import os
import swagger_processor
from resource_manager.uploader import Uploader
from resource_manager.errors import HandledError

AWS_S3_BUCKET_NAME = "CloudGemPortal"
BUCKET_ROOT_DIRECTORY_NAME = "www"

def before_resource_group_updated(hook, resource_group_uploader, **kwargs):
    swagger_path = os.path.join(resource_group_uploader.resource_group.directory_path, 'swagger.json')
    if os.path.isfile(swagger_path):
        resource_group_uploader.context.view.processing_swagger(swagger_path)
        result = swagger_processor.process_swagger_path(swagger_path)
        resource_group_uploader.upload_content('swagger.json', result, 'processed swagger')

def before_project_updated(hook, project_uploader, **kwargs):
    swagger_path = os.path.abspath(os.path.join(__file__, '..', '..', 'project_service_swagger.json'))
    if os.path.isfile(swagger_path):
        project_uploader.context.view.processing_swagger(swagger_path)
        result = swagger_processor.process_swagger_path(swagger_path)
        project_uploader.upload_content('swagger.json', result, 'processed swagger')
    
def after_project_updated(hook, **kwargs):	
    content_path = os.path.abspath(os.path.join(__file__, '..', '..', BUCKET_ROOT_DIRECTORY_NAME))
    upload_project_content(hook.context, content_path)

def upload_project_content(context, content_path):

    if not os.path.isdir(content_path):        
        raise HandledError("Cloud Gem Portal project content not found at {}.".format(content_path))
    
    uploader = Uploader(
        context,
        bucket = context.stack.get_physical_resource_id(context.config.project_stack_id, AWS_S3_BUCKET_NAME),    
        key = ''
    )

    uploader.upload_dir(BUCKET_ROOT_DIRECTORY_NAME, content_path)        
    