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
import datetime
import json

from botocore.exceptions import ClientError

import content_bucket
import dynamic_content_settings
import resource_manager.util
import show_manifest
from cgf_utils import custom_resource_utils
from resource_manager.errors import HandledError


def get_staging_table(context, deployment_name, versioned=False):
    if versioned:
        staging_table_name = dynamic_content_settings.get_default_versioned_staging_table_name()
    else:
        staging_table_name = dynamic_content_settings.get_default_staging_table_name()
    staging_table = _get_staging_table_by_name(context, deployment_name, dynamic_content_settings.get_default_resource_group(), staging_table_name)
    return staging_table


def _get_staging_table_by_name(context, deployment_name,
                               resource_group_name=dynamic_content_settings.get_default_resource_group(),
                               table_name=dynamic_content_settings.get_default_staging_table_name()):
    """Returns the resource id of the staging table."""
    return _get_stack_resource_by_name(context, deployment_name, resource_group_name, table_name)


def _get_stack_resource_by_name(context, deployment_name, resource_group_name, resource_name):
    """Returns the resource id of the staging table."""
    if deployment_name is None:
        deployment_name = context.config.default_deployment

    stack_id = context.config.get_resource_group_stack_id(deployment_name, resource_group_name, optional=True)

    resource_obj = context.stack.get_physical_resource_id(stack_id, resource_name)
    resource_arn = custom_resource_utils.get_embedded_physical_id(resource_obj)
    show_manifest.found_stack(resource_arn)
    return resource_arn


def _get_request_lambda(context):
    return _get_request_lambda_by_name(context, context.config.default_deployment, dynamic_content_settings.get_default_resource_group(),
                                       dynamic_content_settings.get_default_request_lambda_name())


def _get_request_lambda_by_name(context, deployment_name, resource_group_name=dynamic_content_settings.get_default_resource_group(),
                                lambda_name=dynamic_content_settings.get_default_request_lambda_name()):
    """Returns the resource id of the request lambda."""
    return _get_stack_resource_by_name(context, deployment_name, resource_group_name, lambda_name)


def _get_time_format():
    return '%b %d %Y %H:%M'


def get_formatted_time_string(timeval):
    return datetime.datetime.strftime(timeval, _get_time_format())


def get_struct_time(timestring):
    try:
        return datetime.datetime.strptime(timestring, _get_time_format())
    except ValueError:
        raise HandledError('Expected time format {}'.format(get_formatted_time_string(datetime.datetime.utcnow())))


def parse_staging_arguments(args):
    staging_args = {
        'StagingStatus': args.staging_status,
        'StagingStart': args.start_date,
        'StagingEnd': args.end_date
    }

    return staging_args


def command_set_staging_status(context, args):
    staging_args = parse_staging_arguments(args)

    if not args.version_id:
        args.version_id = content_bucket.get_latest_version_id(context, args.file_path, context.config.default_deployment)

    set_staging_status(args.file_path, context, staging_args, context.config.default_deployment, args.version_id)

    if args.include_children:
        children = get_children_by_parent_name_and_version(context, args.file_path, args.version_id)
        for child in children:
            set_staging_status(child.get('FileName', {}).get('S'), context, staging_args, context.config.default_deployment,
                               child.get('VersionId', {}).get('S'))


def get_children_by_parent_name_and_version(context: object, parent_name: str, parent_version_id: str = None) -> list:
    dynamoDB = context.aws.client('dynamodb', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    versioned = content_bucket.content_versioning_enabled(context, context.config.default_deployment)
    table_arn = get_staging_table(context, context.config.default_deployment, versioned)

    response = dynamoDB.scan(
        TableName=table_arn,
        ExpressionAttributeNames={
            "#Parent": "Parent"
        },
        ExpressionAttributeValues={
            ':parentnameval': {
                'S': parent_name
            }
        },
        FilterExpression='#Parent = :parentnameval'
    )
    children_by_parent_name = response.get('Items', [])
    children_by_parent_name_and_version = []
    for child in children_by_parent_name:
        if parent_version_id and (parent_version_id not in child.get('ParentVersionIds', {}).get('SS', [])):
            continue
        children_by_parent_name_and_version.append(child)
    return children_by_parent_name_and_version


def set_staging_status(file_path, context, staging_args, deployment_name, version_id=None):
    staging_status = staging_args.get('StagingStatus')
    staging_start = staging_args.get('StagingStart')
    staging_end = staging_args.get('StagingEnd')
    signature = staging_args.get('Signature')
    parent_pak = staging_args.get('Parent')
    parent_version_id = staging_args.get('ParentVersionId')
    file_size = staging_args.get('Size')
    file_hash = staging_args.get('Hash')

    if staging_status == 'WINDOW':
        if not staging_start:
            raise HandledError('No start-date set for WINDOW staging_status')

        if not staging_end:
            raise HandledError('No end-date set for WINDOW staging_status')

    dynamoDB = context.aws.client('dynamodb', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    versioned = content_bucket.content_versioning_enabled(context, deployment_name)
    table_arn = get_staging_table(context, deployment_name, versioned)
    key = {'FileName': {'S': file_path}} if not versioned else {'FileName': {'S': file_path}, 'VersionId': {'S': version_id}}
    attributeUpdate = {}

    if staging_status:
        attributeUpdate['StagingStatus'] = {
            'Value': {
                'S': staging_status
            }
        }

    if parent_pak and len(parent_pak):
        attributeUpdate['Parent'] = {
            'Value': {
                'S': parent_pak
            }
        }

    if versioned and parent_version_id:
        response = dynamoDB.get_item(
            TableName=table_arn,
            Key=key
        )
        item = response.get('Item', {})
        parent_version_ids = item.get('ParentVersionIds', {}).get('SS', [])
        if parent_version_id not in parent_version_ids:
            parent_version_ids.append(parent_version_id)
            attributeUpdate['ParentVersionIds'] = {
                'Value': {
                    'SS': parent_version_ids
                }
            }

    if signature:
        # Ensure signature from UX is handled
        if isinstance(signature, (bytes, bytearray)):
            signature = signature.decode('utf-8')
        attributeUpdate['Signature'] = {
            'Value': {
                'S': signature
            }
        }
    else:
        attributeUpdate['Signature'] = {
            'Action': 'DELETE'
        }

    if file_size is not None:
        attributeUpdate['Size'] = {
            'Value': {
                'S': str(file_size)
            }
        }

    if file_hash is not None and len(file_hash) > 0:
        attributeUpdate['Hash'] = {
            'Value': {
                'S': str(file_hash)
            }
        }

    if staging_status == 'WINDOW':
        time_start = None
        if staging_start.lower() == 'now':
            time_start = get_formatted_time_string(datetime.datetime.utcnow())
        else:
            # Validate our input
            time_start = get_formatted_time_string(get_struct_time(staging_start))

        time_end = None
        if staging_end.lower() == 'never':
            time_end = get_formatted_time_string(datetime.datetime.utcnow() + datetime.timedelta(days=dynamic_content_settings.get_max_days()))
        else:
            # Validate our input
            time_end = get_formatted_time_string(get_struct_time(staging_end))

        attributeUpdate['StagingStart'] = {
            'Value': {
                'S': time_start
            }
        }

        attributeUpdate['StagingEnd'] = {
            'Value': {
                'S': time_end
            }
        }

    try:
        response = dynamoDB.update_item(
            TableName=table_arn,
            Key=key,
            AttributeUpdates=attributeUpdate,
            ReturnValues='ALL_NEW'
        )
    except Exception as e:
        raise HandledError(
            'Could not update status for'.format(
                FilePath=file_path,
            ),
            e
        )
    item_data = response.get('Attributes', None)

    if not staging_status:
        # Staging status doesn't change
        return

    new_staging_status = {
        'StagingStatus': item_data.get('StagingStatus', {}).get('S', 'UNSET'),
        'StagingStart': item_data.get('StagingStart', {}).get('S'),
        'StagingEnd': item_data.get('StagingEnd', {}).get('S'),
        'Parent': item_data.get('Parent', {}).get('S')
    }

    show_manifest.show_staging_status(file_path, new_staging_status)


def empty_staging_table(context, deleted_object_list):
    dynamoDB = context.aws.client('dynamodb', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    versioned = content_bucket.content_versioning_enabled(context, context.config.default_deployment)
    table_arn = get_staging_table(context, context.config.default_deployment, versioned)
    for this_item in deleted_object_list:
        key_path = this_item.get('Key')
        if versioned:
            remove_entry(context, key_path, this_item.get('VersionId'))
        else:
            remove_entry(context, key_path)


def remove_entry(context, file_path, version_id=None):
    dynamoDB = context.aws.client('dynamodb', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    versioned = content_bucket.content_versioning_enabled(context, context.config.default_deployment)
    table_arn = get_staging_table(context, context.config.default_deployment, versioned)
    key = {'FileName': {'S': file_path}, 'VersionId': {'S': version_id}} if versioned else {'FileName': {'S': file_path}}
    try:
        response = dynamoDB.delete_item(
            TableName=table_arn,
            Key=key
        )
    except Exception as e:
        raise HandledError('Could not delete entry for for'.format(FilePath=file_path, ), e)


def command_request_url(context, args):
    file_name = args.file_path

    file_list = [file_name]

    request = {'FileList': file_list}

    lambdaClient = context.aws.client('lambda', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))

    lambda_arn = _get_request_lambda(context)

    contextRequest = json.dumps(request)
    print('Using request {}'.format(contextRequest))
    try:
        response = lambdaClient.invoke(
            FunctionName=lambda_arn,
            InvocationType='RequestResponse',
            Payload=contextRequest
        )
        payloadstream = response.get('Payload', {})
        payload_string = payloadstream.read()
        print('Got result {}'.format(payload_string))
    except Exception as e:
        raise HandledError('Could not get URL for'.format(FilePath=file_name, ), e)


def signing_status_changed(context, key, do_signing, version_id):
    dynamoDB = context.aws.client('dynamodb', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    versioned = content_bucket.content_versioning_enabled(context, context.config.default_deployment)
    table_arn = get_staging_table(context, context.config.default_deployment, versioned)
    key = {'FileName': {'S': key}} if not versioned else {'FileName': {'S': key}, 'VersionId': {'S': version_id}}

    try:
        response = dynamoDB.get_item(
            TableName=table_arn,
            Key=key
        )

    except ClientError as ce:
        if ce.response['Error']['Code'] == 'ResourceNotFoundException':
            return True
        else:
            raise HandledError('Could not get signing status for {}'.format(key), ce)
    except Exception as e:
        raise HandledError('Failed to get signing status for {}'.format(key), e)

    pak_is_signed = response.get('Item', {}).get('Signature', {}).get('S', '') != ''
    return pak_is_signed != do_signing
