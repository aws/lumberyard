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
from resource_manager.errors import HandledError
import dynamic_content_settings
import os

def _get_access_bucket(context, deployment_name):

    bucket_name = dynamic_content_settings.get_access_bucket_name()
    resource_group_name=dynamic_content_settings.get_default_resource_group()

    '''Returns the resource id of the content bucket.'''
    if deployment_name is None:
        deployment_name = context.config.default_deployment

    stack_id = context.config.get_resource_group_stack_id(deployment_name, resource_group_name, optional=True)
    bucketResource = context.stack.get_physical_resource_id(stack_id, bucket_name)
    return bucketResource

def command_upload_cloudfront_key(context, args):
    key_file_path = args.key_path
    deployment_name = args.deployment_name or None

    if not os.path.isfile(str(key_file_path)):
        raise HandledError('No file at {}'.format(key_file_path))

    base_name = os.path.basename(key_file_path)
    if not base_name.endswith('.pem'):
        raise HandledError('{} is not a .pem file').format(base_name)

    if not base_name.startswith('pk-'):
        raise HandledError('{} does not appear to be a cloudfront key (Expected name format is pk-<accountkey>.pem)'.format(base_name))

    s3 = context.aws.client('s3')

    access_bucket = _get_access_bucket(context, deployment_name)
    if not access_bucket:
        raise HandledError('Could not find access bucket!')

    try:
        s3.upload_file(key_file_path, access_bucket, dynamic_content_settings.get_access_bucket_key_dir() + '/' + base_name)
    except Exception as e:
        raise HandledError('Failed to upload with result {}'.format(e))

    print 'Uploaded key file to {}/{}/{}'.format(access_bucket,dynamic_content_settings.get_access_bucket_key_dir(),base_name)
