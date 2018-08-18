import retry
import boto3
import metric_constant as c
import math
import uuid
import sys
import time
import os
import math
import csv
import util
import sensitivity
import retry
import enum_type
from s3 import S3
from StringIO import StringIO

DEFAULT_EVENTS = enum_type.create(CLIENTINITCOMPLETE="clientinitcomplete", SESSIONSTART="sessionstart")

class Athena(object):

    def __init__(self, db_name, context = {}):        
        self.__context = context                        
        self.__client = boto3.client('athena',region_name=os.environ[c.ENV_REGION], api_version='2017-05-18')        
        self.__db_name = db_name
        self.__bucket = os.environ[c.ENV_S3_STORAGE]
        self.__s3 = S3(bucket=self.__bucket)
  
    @property
    def query_results_path(self):
        return "results"

    def query(self, sql, result_as_list = True, sync=True):      
        if not self.is_valid_query(sql):
            return None

        print "Executing query\n\t", sql        
        params = dict({})                
        params['QueryString'] = sql
        params['QueryExecutionContext']={
            'Database': self.__db_name
        }
        params['ResultConfiguration'] = {
            'OutputLocation': "s3://{}/{}".format(self.__bucket, self.query_results_path),
            'EncryptionConfiguration': {
                'EncryptionOption': 'SSE_S3'
            }
        }        
        response = retry.try_with_backoff(self.__context, self.__client.start_query_execution, **params) 
        id = response['QueryExecutionId']                  
        if sync:  
            #TODO: implement a boto3 waiter
            while True:
                query = self.get_query_execution(id)
                print "Query '{}...' is".format(sql[:30]), query['Status']['State'] 
                if query['Status']['State'] == 'RUNNING' or query['Status']['State'] == 'QUEUED':
                    time.sleep(3)
                elif query['Status']['State'] == 'FAILED':
                    print "The query '{}' FAILED with ERROR: {}".format(query, query['Status']["StateChangeReason"])
                    if 'HIVE_CANNOT_OPEN_SPLIT' in query['Status']["StateChangeReason"]:
                        #The amoeba generator could be running which would cause files to be removed        
                        return []
                    else:                        
                        return None
                else:                    
                    return self.get_output( query['ResultConfiguration']['OutputLocation'], result_as_list)
        else:
            return id

    def is_valid_query(self, sql):
        #To be a valid client query the query must contain a SELECT and FROM operator.
        #Athena only allows one query to be executed at a time.   The Athena compiler would throw an error on this -> select 1; select 2
        required_operators = [['select','from'], ['describe']]
        valid = False
        sql = sql.lower()
        for operator_set in required_operators: 
            is_set_valid = False
            for operator in operator_set:   
                if operator not in sql:
                    is_set_valid = False
                    break
                else:
                    is_set_valid = True
            valid = is_set_valid or valid
        return valid

    def get_named_query(self, name):
        response = self.__client.get_named_query(
            NamedQueryId=name
        )
        return response['NamedQuery']

    def get_query_execution(self, id):    
        params = dict({})                
        params['QueryExecutionId'] = id
        response = retry.try_with_backoff(self.__context, self.__client.get_query_execution, **params)
        return response['QueryExecution']

    def get_output(self, location, result_as_list=True):
        parts = location.split("/")
        file = parts[len(parts)-1]                
        result = StringIO(self.__s3.read("{}/{}".format(self.query_results_path, file)))                
        self.__s3.delete(["/{}/{}".format(self.query_results_path, file)]) 
        if result_as_list:                   
            return list(csv.reader(result, delimiter=',', quotechar='"')) 
        return result.getvalue()

def get_table_prefix(arn, use_cache=True):
    return "{}_".format(get_database_name(arn, use_cache))

def get_database_name(arn, use_cache=True):
    project_name = util.get_project_name(arn, use_cache).replace("-","_").lower()
    deployment_name = util.get_deployment_name(arn, use_cache).replace("-", "_").lower()
    return "{}_{}".format(project_name,deployment_name)

class Query(object):
    def __init__(self, arn):           
        self.__database_name = get_database_name(arn)
        self.__table_prefix = "{}_table_".format(self.__database_name.lower())
        self.__athena = Athena(self.__database_name)

    @property
    def client(self):
        return self.__athena

    @property
    def database_name(self):
        return self.__database_name

    @property
    def prefix(self):
        return self.__table_prefix

    def execute(self, query, result_as_list=True, sync=True):   
        return self.__athena.query(query, result_as_list, sync) 

    def execute_with_format(self, query_format, result_as_list=True, sync=True):   
        return self.__athena.query(query_format.format(self.__database_name, self.__table_prefix), result_as_list, sync) 

 
                
    