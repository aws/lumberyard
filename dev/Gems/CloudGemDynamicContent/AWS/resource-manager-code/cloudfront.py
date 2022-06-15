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
import os

import dynamic_content_settings
from resource_manager.errors import HandledError


def _get_access_bucket(context, deployment_name):
    bucket_name = dynamic_content_settings.get_access_bucket_name()
    resource_group_name = dynamic_content_settings.get_default_resource_group()

    '''Returns the resource id of the content bucket.'''
    if deployment_name is None:
        deployment_name = context.config.default_deployment

    stack_id = context.config.get_resource_group_stack_id(deployment_name, resource_group_name, optional=True)
    bucket_resource = context.stack.get_physical_resource_id(stack_id, bucket_name)
    return bucket_resource


def command_upload_cloudfront_key(context, args):
    key_file_path = args.key_path
    deployment_name = args.deployment_name or None

    if not os.path.isfile(str(key_file_path)):
        raise HandledError('No file at {}'.format(key_file_path))

    base_name = os.path.basename(key_file_path)
    if not base_name.endswith('.pem'):
        raise HandledError('{} is not a .pem file'.format(base_name))

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

    print('Uploaded key file to {}/{}/{}'.format(access_bucket, dynamic_content_settings.get_access_bucket_key_dir(), base_name))


def get_cloudfront_distribution():
    return 'ContentDistribution'


def command_invalidate_file(context, args):
    create_invalidation(context, args.file_path, args.caller_reference)


def create_invalidation(context, file_path, caller_reference):
    if not file_path.startswith('/'):
        # Add '/' at the start of the file path if not exists since it's required by the create_invalidation API
        file_path = '/' + file_path

    distribution_id = _get_distribution_id(context)
    if not distribution_id:
        print('No CloudFront distribution is found. Invalidation request will be ignored')
        return

    client = context.aws.client('cloudfront')
    try:
        client.create_invalidation(
            DistributionId=distribution_id,
            InvalidationBatch={
                'Paths': {
                    'Quantity': 1,
                    'Items': [
                        file_path
                    ]
                },
                'CallerReference': caller_reference
            })
        print('File {} is removed from CloudFront edge caches.'.format(file_path))
    except Exception as e:
        raise HandledError('Failed to invalidate file {} with result {}'.format(file_path, e))


def _get_distribution_id(context):
    return _get_distribution_id_by_name(context, context.config.default_deployment, dynamic_content_settings.get_default_resource_group(),
                                        get_cloudfront_distribution())


def _get_distribution_id_by_name(context, deployment_name,
                                 resource_group_name=dynamic_content_settings.get_default_resource_group(),
                                 distribution_name=get_cloudfront_distribution()):
    """Returns the resource id of the cloudfront distribution."""
    if deployment_name is None:
        deployment_name = context.config.default_deployment

    stack_id = context.config.get_resource_group_stack_id(deployment_name, resource_group_name, optional=True)
    distribution_id = context.stack.get_physical_resource_id(stack_id, distribution_name)
    return distribution_id
