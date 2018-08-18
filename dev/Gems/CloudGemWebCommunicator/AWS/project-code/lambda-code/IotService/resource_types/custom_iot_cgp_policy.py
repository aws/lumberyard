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
from cgf_utils import custom_resource_response
from cgf_utils import properties
from cgf_utils import aws_utils
from resource_manager_common import stack_info
from iot_policy_shared import detach_policy_principals
import boto3
import json

iot_client = aws_utils.ClientWrapper(boto3.client('iot'))

def handler(event, context):

    # Validate RequestType
    request_type = event['RequestType']
    if request_type not in ['Create', 'Update', 'Delete']:
        raise RuntimeError('Unexpected request type: {}'.format(request_type))

    stack_arn = event['StackId']
    stack_manager = stack_info.StackInfoManager()
    stack = stack_manager.get_stack_info(stack_arn)

    # Physical ID is always the same.
    physical_resource_id = aws_utils.get_stack_name_from_stack_arn(stack_arn) + '-' + event['LogicalResourceId']

    if request_type == 'Delete':
        _delete_iot_cgp_policy(physical_resource_id)
    elif request_type == 'Update':
        _update_iot_cgp_policy(physical_resource_id, stack)
    elif request_type == 'Create':
        _create_iot_cgp_policy(physical_resource_id, stack)

    return custom_resource_response.success_response({}, physical_resource_id)
    
def _delete_iot_cgp_policy(physical_resource_id):

    try:
        find_policy_response = iot_client.get_policy(policyName=physical_resource_id)
    except Exception as e:
        # Wrapper should have logged error. Return gracefully rather than raising
        return e

    detach_policy_principals(physical_resource_id)
    
    response = iot_client.delete_policy(policyName=physical_resource_id)

    return response

def _update_iot_cgp_policy(physical_resource_id, stack):

    # delete the existing policy rather than increment versions - there's a 5 version limit and we don't
    # currently use old versions for anything
    _delete_iot_cgp_policy(physical_resource_id)

    _create_iot_cgp_policy(physical_resource_id, stack)

def _create_iot_cgp_policy(physical_resource_id, stack):

    iot_client.create_policy(policyName=physical_resource_id, policyDocument=_get_cgp_listener_policy(stack))

def _get_cgp_listener_policy(stack):
    account_id = aws_utils.ClientWrapper(boto3.client('sts')).get_caller_identity()['Account']

    iot_client_resource = "arn:aws:iot:{}:{}:client/${{cognito-identity.amazonaws.com:sub}}".format(stack.region, account_id)
    policy_doc = '''{
      "Version": "2012-10-17",
      "Statement": [
        {
          "Effect": "Allow",
          "Action": [
            "iot:Connect",
            "iot:Receive",
            "iot:Subscribe",
            "iot:Publish"
          ],
          "Resource": [
            "*"
          ]
        }
      ]
    }'''

    return policy_doc

