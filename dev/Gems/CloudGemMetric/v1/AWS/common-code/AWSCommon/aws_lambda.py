import retry
import boto3
import metric_constant as c
import time
import os
import json

class Lambda(object):

    def __init__(self, context = {}):        
        self.__context = context        
        self.__client = boto3.client('lambda',region_name=os.environ[c.ENV_REGION], api_version='2015-03-31')     

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