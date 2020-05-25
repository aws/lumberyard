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
import os
import metric_constant as c


def client(service_id, api_version=None, region=None, aws_access_key_id=None, aws_secret_access_key=None):
    region = get_env_variables(c.ENV_REGION, region, None)
    aws_access_key_id = get_env_variables("AWS_ACCESS_KEY", aws_access_key_id, None)
    aws_secret_access_key = get_env_variables("AWS_SECRET_KEY", aws_secret_access_key, None)
    
    if region is None:
        if aws_access_key_id is None and aws_secret_access_key is None: 
            return boto3.client(service_id, api_version=api_version)
        else:
            return boto3.client(service_id, api_version=api_version, aws_access_key_id=aws_access_key_id, aws_secret_access_key=aws_secret_access_key)
    else:
        if aws_access_key_id is None and aws_secret_access_key is None: 
            return boto3.client(service_id, region_name=region, api_version=api_version)
        else:
            return boto3.client(service_id, api_version=api_version, region_name=region, aws_access_key_id=aws_access_key_id, aws_secret_access_key=aws_secret_access_key)


def resource(service_id, api_version=None, region=None, aws_access_key_id=None, aws_secret_access_key=None):
    region = get_env_variables(c.ENV_REGION, region, None)
    aws_access_key_id = get_env_variables("AWS_ACCESS_KEY", aws_access_key_id, None)
    aws_secret_access_key = get_env_variables("AWS_SECRET_KEY", aws_secret_access_key, None)
    
    if region is None:
        if aws_access_key_id is None and aws_secret_access_key is None: 
            return boto3.resource(service_id, api_version=api_version)
        else:
            return boto3.resource(service_id, api_version=api_version, aws_access_key_id=aws_access_key_id, aws_secret_access_key=aws_secret_access_key)
    else:
        if aws_access_key_id is None and aws_secret_access_key is None: 
            return boto3.resource(service_id, region_name=region, api_version=api_version)
        else:
            return boto3.resource(service_id, api_version=api_version, region_name=region, aws_access_key_id=aws_access_key_id, aws_secret_access_key=aws_secret_access_key)

def get_env_variables(id, variable, default):
    if variable:
        return variable
    elif id in os.environ:
        return os.environ[id]
    else:
        return default
        
        
        