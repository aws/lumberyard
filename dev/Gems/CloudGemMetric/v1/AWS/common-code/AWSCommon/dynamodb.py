from random import sample
from botocore.exceptions import ClientError
from boto3.dynamodb.conditions import Key
from boto3.dynamodb.conditions import Attr
from boto3 import resource
import metric_constant as c
import boto3
import uuid
import retry
import time
import util
import random 
import datetime
import os
import errors
import json

class DynamoDb(object):
    def __init__(self, context={}):                 
        self.__telemetry_table = None
        self.__context_table = None
        self.__context = context
        self.__resource = boto3.resource('dynamodb', region_name=os.environ[c.ENV_REGION], api_version='2012-08-10')    
        self.__client = boto3.client('dynamodb', region_name=os.environ[c.ENV_REGION], api_version='2012-08-10')
        self.update_context()

    @property
    def context_table(self):        
        if self.__context_table is None:
            self.__context_table = self.__resource.Table(os.environ[c.ENV_DB_TABLE_CONTEXT]) 
        return self.__context_table

    @property
    def avg_delete_duration(self):
        if c.DB_ATTR_DELETE_DURATION in self.__context:            
            return self.__context[c.DB_ATTR_DELETE_DURATION] 
        return 0

    def get(self, table, params={}):
        response = retry.try_with_backoff(self.__context, table.scan, **params)
        return json.loads(json.dumps(response, cls=util.DynamoDbDecoder))

    def get_key(self, key):
        params = dict({}) 
        params["KeyConditionExpression"] = Key('key').eq(key)   
        response = retry.try_with_backoff(self.__context, self.context_table.query, **params)
        return json.loads(json.dumps(response, cls=util.DynamoDbDecoder))

    def update_context(self):
        response = self.get(self.context_table)        
        if response and len(response['Items']) >= 0:
            arr = json.loads(json.dumps(response['Items'], cls=util.DynamoDbDecoder))                 
            for pair in arr:                                                
                key = pair['key']                
                self.__context[key]= pair['value']

    def set(self, key, value):
        params = dict({})
        params["UpdateExpression"]= "SET #val = :val"       
        params["ExpressionAttributeNames"]={
            '#val': c.KEY_SECONDARY                      
        }
        params["ExpressionAttributeValues"]={                    
            ':val': value
        }                 
        params["Key"]={ 
            c.KEY_PRIMARY: key
        }          
        try:
            return self.update(self.context_table.update_item, params)        
        except Exception as e:
            raise errors.ClientError("Error updating the context parameter '{}' with value '{}'.\nError: {}".format(key, value, e))
    
    def delete_item(self, key):
        params = dict({})                
        params["Key"]={ 
            c.KEY_PRIMARY: key
        }          
        self.__try(True, self.context_table.delete_item, params)

    def update(self, func, params):                   
        response = self.__try(True, func, params)
        return response

    def __try(self, retry, func, params):               
        attempts = 0
        backoff = 1  
        while True:  
            try:
                return func(
                    **params
                )
            except ClientError as e:
                attempts += 1
                if e.response['Error']['Code'] == 'ConditionalCheckFailedException' or e.response['Error']['Code'] == 'ProvisionedThroughputExceededException':                    
                    if not retry:
                        return None                 
                    print "\n\n[ConditionalCheckFailedException] Func: {} Retry: {}\n\n".format(func, retry)
                    backoff = min(self.__context[c.KEY_BACKOFF_MAX_SECONDS], random.uniform(self.__context[c.KEY_BACKOFF_BASE_SECONDS], backoff * 3.0))
                    if self.__break_retry(backoff, attempts):
                        raise e    
                    print "Sleeping {} seconds before attempting the {} request again.".format(backoff, func)
                    time.sleep(backoff)                 
                else:                    
                    raise e
    
    def __break_retry(self, backoff, current_attempt_number):        
        time_remaining = util.time_remaining(self.__context)
        print "The lambda has {} seconds remaining. It started at {}.".format(time_remaining, datetime.datetime.fromtimestamp(self.__context[c.KEY_START_TIME]).strftime('%Y-%m-%d %H:%M:%S'))        
        if time_remaining - backoff < context[c.CW_ATTR_SAVE_DURATION] or current_attempt_number >= self.__context[c.KEY_BACKOFF_MAX_TRYS] :
            print "Breaking the lock attempt as the lambda is nearing timeout or maximum retry limit is hit.  The lambda has {} seconds remaining.  There have been {} attempts made.".format(time_remaining,current_attempt_number)
            return True;
        return False




