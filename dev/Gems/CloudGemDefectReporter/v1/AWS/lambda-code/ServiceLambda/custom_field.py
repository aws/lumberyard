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

from __future__ import print_function
import boto3
import CloudCanvas
import json

from botocore.exceptions import ClientError

def __get_bucket():
    if not hasattr(__get_bucket,'client_configuration'):
        __get_bucket.client_configuration = boto3.resource('s3').Bucket(CloudCanvas.get_setting("ClientConfiguration"))
    return __get_bucket.client_configuration

def update_client_configuration(client_configuration):
    __get_bucket().put_object(Key="client_configuration.json", Body=json.dumps(client_configuration))
    return 'SUCCEED'

def get_client_configuration():
    client = boto3.client('s3')
    client_configuration = []
    try:
        response = client.get_object(Bucket=CloudCanvas.get_setting("ClientConfiguration"), Key="client_configuration.json")
        client_configuration = json.loads(response["Body"].read())
    except ClientError as e:
        if e.response['Error']['Code'] == 'AccessDenied':
            client_configuration = []
        else:
            raise e

    return client_configuration

    
