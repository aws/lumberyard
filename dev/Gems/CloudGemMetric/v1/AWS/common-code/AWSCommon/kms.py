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
import retry
import boto3_util
import metric_constant as c
import math
import uuid
import sys
import time
import os
import math
from botocore.exceptions import ClientError

class Kms(object):

    def __init__(self):                
        self.__client = boto3.client('kms', api_version='2014-11-01')

    def encrypt(self, data):        
        params = dict({})
        params['KeyId'] = c.KMS_KEY_ID
        params['Plaintext'] = data
        params['EncryptionContext'] = {}                
        response = self.__client.encrypt(**params)
        print("encrypt", response)
        return response

    def create_key(self):        
        params = dict({})      
        response = self.__client.create_key(**params)
        print("create key", response)
        return response['KeyMetadata']

    def create_key_alias(self, key_id, alias_name):        
        params = dict({})
        params['AliasName'] = self.__alias_name(alias_name)
        params['TargetKeyId'] = key_id      
        response = self.__client.create_alias(**params)
        print("create alias", response)
        return response
    
    def list_keys(self):        
        params = dict({})   
        response = self.__client.list_keys(**params)
        print("list_keys", response)
        return response['Keys']

    def list_aliases(self):        
        params = dict({})   
        response = self.__client.list_aliases(**params)
        print("list_aliases", response)
        return response['Aliases']

    def alias_exists(self, alias_name):        
        aliases = self.list_aliases()
        for alias in aliases:
            if alias['AliasName'] == self.__alias_name(alias_name):
                print("alias exists: {}".format(alias_name))
                return True
        return False        

    def get_key(self):        
        keys = self.list_keys()
        if len(keys) == 0:
            return None        
        return keys[0]['KeyId']

    def __alias_name(self, alias_name):
        return "alias/{}".format(alias_name)