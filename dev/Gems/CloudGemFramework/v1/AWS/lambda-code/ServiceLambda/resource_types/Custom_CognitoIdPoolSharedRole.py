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

import boto3
import botocore
import os
import json
from cgf_utils import aws_utils
from cgf_utils import custom_resource_response
from resource_manager_common import stack_info
from cgf_utils import properties
from resource_types.cognito import identity_pool

from botocore.client import Config



def handler(event, context):
    if event['RequestType'] == 'Delete':
        return custom_resource_response.success_response({'Arn': ''}, '')
    props = properties.load(event, {
            'ConfigurationBucket': properties.String(),
            'ConfigurationKey': properties.String(),
            'LogicalPoolName': properties.String(),
            'RoleType': properties.String(default=""),
            'Path': properties.String(),
            'AssumeRolePolicyDocument': properties.Dictionary()
        })

    stack_manager = stack_info.StackInfoManager()
    stack = stack_manager.get_stack_info(event['StackId'])

    identity_client = identity_pool.get_identity_client()

    cognito_pool_info = aws_utils.get_cognito_pool_from_file(props.ConfigurationBucket,
        props.ConfigurationKey,
        props.LogicalPoolName,
        stack)

    arn = ''
    if cognito_pool_info:
        response = identity_client.get_identity_pool_roles(IdentityPoolId=cognito_pool_info['PhysicalResourceId'])
        arn = response.get("Roles", {}).get(props.RoleType, "")
    else:
        name = "{}{}Role".format(stack.stack_name, event['LogicalResourceId'])
        arn=create_role(name, props)

    return custom_resource_response.success_response({'Arn': arn}, arn)


def create_role(role_name, props):
    iam_client = aws_utils.ClientWrapper(boto3.client('iam'))
    response = iam_client.create_role(
        Path=props.Path,
        RoleName=role_name,
        AssumeRolePolicyDocument=json.dumps(props.AssumeRolePolicyDocument)
        )
    return response['Role']['Arn']

