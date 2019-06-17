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

from cgf_utils import custom_resource_response
from cgf_utils import aws_utils
from cgf_utils import reference_type_utils
from cgf_utils import properties
from resource_manager_common import stack_info

def handler(event, context):
    stack_arn = event['StackId']
    stack = stack_info.StackInfoManager().get_stack_info(stack_arn)
    props = properties.load(event, {
        'ReferenceName': properties.String()
    })

    request_type = event['RequestType']
    if request_type not in ['Create', 'Update', 'Delete']:
        raise RuntimeError('Unexpected request type: {}'.format(request_type))

    data = {}
    if request_type != 'Delete':
        data = {
            'PhysicalId': _get_reference_physical_id(stack, props.ReferenceName)
        }

    physical_resource_id = aws_utils.construct_custom_physical_resource_id_with_data(stack_arn, event['LogicalResourceId'], {'ReferenceName': props.ReferenceName})

    return custom_resource_response.success_response(data, physical_resource_id)

def arn_handler(event, context):
    stack = stack_info.StackInfoManager().get_stack_info(event['StackId'])
    reference_name = aws_utils.get_data_from_custom_physical_resource_id(event['ResourceName']).get('ReferenceName')

    result = {
        'Arn': _get_reference_arn(stack, reference_name)
    }

    return result

def _get_reference_arn(stack, reference_name):
    reference_metadata = reference_type_utils.get_reference_metadata(stack.project_stack.configuration_bucket, stack.project_stack.project_name, reference_name)
    return reference_metadata.get('Arn')

def _get_reference_physical_id(stack, reference_name):
    reference_metadata = reference_type_utils.get_reference_metadata(stack.project_stack.configuration_bucket, stack.project_stack.project_name, reference_name)
    return reference_metadata.get('PhysicalId')
