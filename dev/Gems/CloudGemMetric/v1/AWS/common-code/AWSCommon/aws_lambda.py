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

import retry
import metric_constant as c
import time
import os
import json
import boto3_util

class Lambda(object):

    def __init__(self, context = {}):        
        self.__context = context        
        self.__client = boto3_util.client('lambda', api_version='2015-03-31')

    def invoke(self, func_name, payload = {} ):
        return self.__client.invoke(
            FunctionName=func_name,
            InvocationType='Event',
            Payload=json.dumps(payload)
        )
    
    def invoke_sync(self, func_name, payload = {} ):
        return self.__client.invoke(
            FunctionName=func_name,
            InvocationType='RequestResponse',
            Payload=json.dumps(payload)
        )