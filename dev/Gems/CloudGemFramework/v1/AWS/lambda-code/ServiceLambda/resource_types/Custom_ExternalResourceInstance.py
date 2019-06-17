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
# $Revision: #3 $

import boto3
import json

from cgf_utils import custom_resource_response
from cgf_utils import aws_utils
from cgf_utils import reference_type_utils
from resource_manager_common import stack_info

s3_client = aws_utils.ClientWrapper(boto3.client('s3'))

def handler(event, context):
    stack_arn = event['StackId']
    stack = stack_info.StackInfoManager().get_stack_info(stack_arn)

    if not stack.is_project_stack:
        raise RuntimeError("Custom::ExternalResourceInstance can only be defined in the project stack.")

    request_type = event['RequestType']
    if request_type not in ['Create', 'Update', 'Delete']:
        raise RuntimeError('Unexpected request type: {}'.format(request_type))

    if request_type in ['Create', 'Update']:
        _create_reference_metadata(event, stack)
    else:
        _delete_reference_metadata(event['LogicalResourceId'], stack)

    physical_resource_id = aws_utils.get_stack_name_from_stack_arn(stack_arn) + '-' + event['LogicalResourceId']

    return custom_resource_response.success_response({}, physical_resource_id)

def _delete_reference_metadata(resource_name, stack):
    reference_metadata_key = reference_type_utils.get_reference_metadata_key(stack.project_stack.project_name, resource_name)
    s3_client.delete_object(
        Bucket = stack.project_stack.configuration_bucket,
        Key = reference_metadata_key
    )

def _create_reference_metadata(event, stack):
    _validate_reference_metadata(event)

    reference_metadata = event['ResourceProperties']['ReferenceMetadata']
    resource_name = event['LogicalResourceId']

    reference_metadata_key = reference_type_utils.get_reference_metadata_key(stack.project_stack.project_name, resource_name)
    s3_client.put_object(Bucket=stack.project_stack.configuration_bucket, Key=reference_metadata_key, Body=json.dumps(reference_metadata, indent=2))

def _validate_reference_metadata(event):
    reference_type_utils.load_reference_metadata_properties(event)
