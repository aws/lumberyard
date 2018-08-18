import retry
import boto3
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
        self.__client = boto3.client('kms',region_name=os.environ[c.ENV_REGION], api_version='2014-11-01')                

    def encrypt(self, data):        
        params = dict({})
        params['KeyId'] = c.KMS_KEY_ID
        params['Plaintext'] = data
        params['EncryptionContext'] = {}                
        response = self.__client.encrypt(**params)
        print "encrypt", response
        return response

    def create_key(self):        
        params = dict({})      
        response = self.__client.create_key(**params)
        print "create key", response
        return response['KeyMetadata']

    def create_key_alias(self, key_id, alias_name):        
        params = dict({})
        params['AliasName'] = self.__alias_name(alias_name)
        params['TargetKeyId'] = key_id      
        response = self.__client.create_alias(**params)
        print "create alias", response
        return response
    
    def list_keys(self):        
        params = dict({})   
        response = self.__client.list_keys(**params)
        print "list_keys", response
        return response['Keys']

    def list_aliases(self):        
        params = dict({})   
        response = self.__client.list_aliases(**params)
        print "list_aliases", response
        return response['Aliases']

    def alias_exists(self, alias_name):        
        aliases = self.list_aliases()
        print 
        for alias in aliases:
            if alias['AliasName'] == self.__alias_name(alias_name):
                return True
        return False        

    def get_key(self):        
        keys = self.list_keys()
        if len(keys) == 0:
            return None        
        return keys[0]['KeyId']

    def __alias_name(self, alias_name):
        return "alias/{}".format(alias_name)