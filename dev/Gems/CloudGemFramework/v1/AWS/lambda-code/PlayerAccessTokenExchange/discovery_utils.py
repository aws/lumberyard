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
import json

import boto3
from cgf_utils import aws_utils

lambda_name = os.environ.get('AWS_LAMBDA_FUNCTION_NAME')
current_region = os.environ.get('AWS_REGION')

class StackS3Configuration(object):
    stack_name = ""
    configuration_bucket = ""

def get_configuration_bucket():
    configuration = StackS3Configuration()

    cloud_formation_client = aws_utils.ClientWrapper(boto3.client('cloudformation', region_name=current_region))

    stack_definitions = cloud_formation_client.describe_stack_resources(PhysicalResourceId = lambda_name)
    for stack_definition in stack_definitions['StackResources']:
            if stack_definition.get('LogicalResourceId',None) == 'Configuration':
                configuration.configuration_bucket = stack_definition['PhysicalResourceId']
                configuration.stack_name = stack_definition['StackName']
                break
    return configuration
