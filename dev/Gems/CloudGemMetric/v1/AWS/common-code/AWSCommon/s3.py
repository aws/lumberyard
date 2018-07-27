import retry
import boto3
import metric_constant as c
import math
import uuid
import sys
import time
import os
import math
import util
from botocore.exceptions import ClientError

class S3(object):

    def __init__(self, context={}, bucket=""):        
        self.__context = context        
        self.__resource = boto3.resource('s3',region_name=os.environ[c.ENV_REGION], api_version='2006-03-01')
        self.__client = boto3.client('s3',region_name=os.environ[c.ENV_REGION], api_version='2006-03-01')
        self.__bucket = bucket
  
    @property
    def client(self):
        return self.__client

    def list(self, prefix = None):
        paginator = self.__client.get_paginator( "list_objects_v2" )
        params = dict({})
        params['Bucket'] = self.__bucket        
        params['PaginationConfig'] = {            
            'PageSize': 500
        }
        if prefix is not None:
            params['Prefix'] = prefix
        page_iterator = paginator.paginate(**params)
        return page_iterator

    def delete(self, keys_to_delete):        
        split_size = 1000              
        keys_to_delete = util.split(keys_to_delete, split_size, self.__delete_wrapper )
        for set in keys_to_delete:                                                          
            params = dict({})
            params['Bucket'] = self.__bucket
            params['Delete'] = { 
                    "Objects": set                    
            }            
            response = self.__client.delete_objects(**params)            

    def read(self, key):        
        response = self.__client.get_object(
            Bucket=self.__bucket,
            Key=key            
        )
        return response["Body"].read()
        
    def size(self, key):        
        try:
            response = self.__client.head_object(Bucket=self.__bucket, Key=key)
        except ClientError as e:             
            raise e
                
        return response['ContentLength']

    def size_in_megabytes(self, key):
        return self.size(key)/ 1048576.0

    def __delete_wrapper(self, key):
        return { "Key": key }
        
        
    
                
    