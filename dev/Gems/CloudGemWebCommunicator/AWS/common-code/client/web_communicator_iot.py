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

from cgf_utils import aws_utils
import boto3


def get_iot_client():
    '''Returns a Iot client with proper configuration.'''
    if not hasattr(get_iot_client, 'client'):
        get_iot_client.client = aws_utils.ClientWrapper(boto3.client('iot', api_version='2015-05-28'))

    return get_iot_client.client

def get_iot_data_client():
    '''Returns a IotData client with proper configuration.'''
    if not hasattr(get_iot_data_client, 'client'):
        get_iot_data_client.client = aws_utils.ClientWrapper(boto3.client('iot-data', api_version='2015-05-28'))

    return get_iot_data_client.client

