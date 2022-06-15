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

from botocore.exceptions import ClientError

import dynamic_content_settings
import resource_manager.util
from resource_manager.errors import HandledError

LIST_OBJECT_LIMIT = 1000


def get_content_bucket(context: object) -> str:
    """ 
        Returns the resource id of the default content bucket
    
        Arguments
            context -- context to use
    """
    return get_content_bucket_by_name(context, context.config.default_deployment, dynamic_content_settings.get_default_resource_group(),
                                      dynamic_content_settings.get_default_bucket_name())


def get_content_bucket_by_name(context: object, deployment_name: str,
                               resource_group_name: str = dynamic_content_settings.get_default_resource_group(),
                               bucket_name: str = dynamic_content_settings.get_default_bucket_name()) -> str:
    """ 
        Returns the resource id of the content bucket
    
        Arguments
            context -- context to use
            deployment_name -- name of the deployment
            resource_group_name -- name of the resource group
            bucket_name -- name of the bucket
        """
    if not deployment_name:
        deployment_name = context.config.default_deployment

    stack_id = context.config.get_resource_group_stack_id(deployment_name, resource_group_name, optional=True)
    bucket_resource = context.stack.get_physical_resource_id(stack_id, bucket_name)
    return bucket_resource


def content_versioning_enabled(context: object, deployment_name: str) -> bool:
    """
        Check whether content versioning is enabled.

        Arguments
            context -- context to use
            deployment_name -- name of the deployment
    """
    s3 = context.aws.client('s3', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    response = s3.get_bucket_versioning(
        Bucket=get_content_bucket_by_name(context, deployment_name)
    )
    return response.get('Status', '') == 'Enabled'


def list_all_versions(context: object, deployment_name: str = '') -> dict:
    """
        Return mappings from an s3 object name to all its versions found in the content bucket

        Arguments
            context -- context to use
            deployment_name -- name of the deployment
    """
    bucket_name = get_content_bucket_by_name(context, deployment_name)
    s3 = context.aws.client('s3')

    file_to_versions = {}
    response = s3.list_object_versions(Bucket=bucket_name)
    while True:
        versions = response.get('Versions', []) + response.get('DeleteMarkers', [])
        for version in versions:
            if version['Key'] not in file_to_versions:
                file_to_versions[version['Key']] = [version]
            else:
                file_to_versions[version['Key']].append(version)

        list_object_versions_args = {'Bucket': bucket_name}
        next_key_marker = response.get('NextKeyMarker')
        if next_key_marker:
            list_object_versions_args['KeyMarker'] = next_key_marker
        next_version_id_marker = response.get('NextVersionIdMarker')
        if next_version_id_marker:
            list_object_versions_args['VersionIdMarker'] = next_version_id_marker
        if not next_key_marker and not next_version_id_marker:
            break

        response = s3.list_object_versions(**list_object_versions_args)
    return file_to_versions


def get_bucket_content_list(context: object) -> list:
    """
        list the latest version of files found in the content bucket

        Arguments
            context -- context to use
            deployment_name -- name of the deployment
    """

    s3 = context.aws.client('s3')
    bucket_name = get_content_bucket(context)
    next_marker = 0
    contents_list = []
    while True:
        try:
            res = s3.list_objects(
                Bucket=bucket_name,
                Marker=str(next_marker)
            )
            this_list = res.get('Contents', [])
            contents_list += this_list
            if len(this_list) < get_list_objects_limit():
                break
            next_marker += get_list_objects_limit()
        except Exception as e:
            raise HandledError('Could not list_objects on '.format(bucket=bucket_name), e)
    return contents_list


def delete_objects_from_bucket(context: object, object_list: list):
    """
        Delete a list of objects found in the content bucket

        Arguments
            context -- context to use
            object_list -- a list of objects to delete
    """

    s3 = context.aws.client('s3')
    bucket_name = get_content_bucket(context)
    try:
        res = s3.delete_objects(
            Bucket=bucket_name,
            Delete={'Objects': object_list}
        )
    except Exception as e:
        raise HandledError('Could not delete_objects on '.format(bucket=bucket_name), e)


def get_latest_version_id(context: object, file_key: str, deployment_name: str) -> str:
    """
        Get the latest version ID of an S3 object

        Arguments
            context -- context to use
            file_key -- key of the S3 object
            deployment_name -- name of the deployment
    """
    bucket_name = get_content_bucket_by_name(context, deployment_name)
    s3 = context.aws.client('s3')

    try:
        response = s3.head_object(
            Bucket=bucket_name,
            Key=file_key
        )
    except ClientError as e:
        if e.response['Error']['Code'] == 'NoSuchKey':
            return ""
        else:
            raise e

    return response.get('VersionId')


def is_latest_version(context: object, file_key: str, file_version: str, deployment_name: str) -> bool:
    """
        Check whether a specific version of object is latest

        Arguments
            context -- context to use
            file_key -- key of the S3 object
            file_version -- version of the file to check
            deployment_name -- name of the deployment
    """
    return get_latest_version_id(context, file_key, deployment_name) == file_version


def get_list_objects_limit():
    """
        Return the AWS internal limit on list_objects
    """
    return LIST_OBJECT_LIMIT
