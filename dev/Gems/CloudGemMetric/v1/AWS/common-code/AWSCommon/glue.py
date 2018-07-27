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

class Glue(object):

    def __init__(self):                   
        self.__client = boto3.client('glue',region_name=os.environ[c.ENV_REGION], api_version='2017-03-31')                
  
    def get_crawler_name(self, arn):
        return self.__name("{}".format(util.get_stack_prefix(arn)).replace("-","_").lower())

    def __name(self, name):
        return name[0:75].lower()

    def create_crawler(self, name, role, dbname, table_prefix="auto_", srcs=None, schedule=None):        
        params = dict({})
        params['Name'] = "{}".format(self.__name(name)).lower()
        params['Role'] = role
        params['DatabaseName'] = dbname
        #schedule every hour
        if schedule:
            params['Schedule'] = schedule #"Cron(0 0/1 * * ? *)"  
        params['TablePrefix'] = table_prefix
        params['Targets'] = {
            'S3Targets': srcs
        }
        params['Configuration'] = '{ "Version": 1.0, "CrawlerOutput": { "Partitions": { "AddOrUpdateBehavior": "InheritFromTable" } }}'

        response = retry.try_with_backoff({}, self.__client.create_crawler, **params)
        return response

    def update_crawler(self, name, role, dbname, table_prefix="auto_", srcs=None, schedule=None):        
        params = dict({})
        params['Name'] = "{}".format(self.__name(name)).lower()
        params['Role'] = role
        params['DatabaseName'] = dbname
        #schedule every hour
        if schedule:
            params['Schedule'] = schedule #"Cron(0 0/1 * * ? *)"  
        params['TablePrefix'] = table_prefix
        if srcs:
            params['Targets'] = {
                'S3Targets': srcs
            }
        params['Configuration'] = '{ "Version": 1.0, "CrawlerOutput": { "Partitions": { "AddOrUpdateBehavior": "InheritFromTable" } }}'
             
        response = retry.try_with_backoff({}, self.__client.update_crawler, **params)       
        return response

    def delete_crawler(self, name):
        while True:        
            if self.get_crawler(name) is not None:
                try:
                    response = self.__client.delete_crawler(
                        Name=self.__name(name)
                    )
                except ClientError as e:
                    print e
                    if e.response['Error']['Code'] != 'CrawlerRunningException':                    
                        break                    
            else:
                break
            time.sleep(10)
        return

    def create_database(self, name):        
        response = self.__client.create_database(            
            DatabaseInput={
                'Name': name,
                'Description': 'The database containing all of your Lumberyard game metrics.'               
            }
        )
        database = self.get_database(name)        
        return database['Database']['Name']

    def delete_database(self,name):
        try:            
            result = self.__client.delete_database(            
                Name=name
            )            
        except ClientError as e:
            print e
            return

    def delete_table(self, db_name, table_name):        
        try:            
            result = self.__client.delete_table( 
                DatabaseName=db_name,        
                Name=table_name
            )                    
            print result
        except ClientError as e:
            print e
            return

    def get_database(self, name):
        return self.__client.get_database(           
            Name=name
        )
        
    def start_crawler(self, name, sync=True):
        try:
            response = self.__client.start_crawler(
                Name=self.__name(name)
            )            
        except ClientError as e:
            print e
        
        if sync:
            self.__wait_for_crawler(name)         

    def stop_crawler(self, name, sync=True):
        try:
            response = self.__client.stop_crawler_schedule(
                CrawlerName=self.__name(name)
            )            
        except ClientError as e:
            print e
        if sync:
            self.__wait_for_crawler(name)

    def get_events(self):
        crawler_name = self.get_crawler_name(os.environ[c.ENV_DEPLOYMENT_STACK_ARN])    
        response = self.get_crawler(crawler_name)
        events = []
        if response is not None:
            bucket = "s3://{}/".format(os.environ[c.RES_S3_STORAGE])                        
            if len(response['Crawler']['Targets']['S3Targets']) > 0:
                for s3target in response['Crawler']['Targets']['S3Targets']:
                    event = s3target['Path'].replace(bucket,'')
                    if "=" in event:
                        events.append(event.split("=")[1])
        return events

    def get_crawler(self, name): 
        params = dict({}) 
        params['Name'] = name    
        try:       
            response = self.__client.get_crawler(**params)                    
        except ClientError as e:     
            print e       
            if (hasattr(e, 'response') and e.response and e.response['Error']['Code'] == 'EntityNotFoundException'):
                return None
            raise e
        return response

    def get_databases(self, token=None):                    
        params = dict({})     
        if token:
            params['NextToken'] = token    
        response =  self.__client.get_databases(**params)   
        return response['DatabaseList'] if 'DatabaseList' in response else [], response['NextToken'] if 'NextToken' in response else None

    
    def database_exists(self, db_name):
        db_exists = False
        token = None
        while True:
            databases, token = self.get_databases(token)
            for database in databases:
                if database['Name'].lower() == db_name.lower():
                    db_exists = True
                    token = None
            if token is None:
                break
        return db_exists

    def create_table(self, db_name, table_name, schema, partitions, s3_path):
        params = self.__table_params(db_name, table_name, schema, partitions, s3_path)       
        return self.__client.create_table(**params)

    def update_table_schema(self, db_name, table_name, schema, partitions, s3_path):
        params = self.__table_params(db_name, table_name, schema, partitions, s3_path)       
        return self.__client.update_table(**params)


    def get_partitions(self, db_name, table_name, token):      
        params = dict({})     
        params['DatabaseName'] = db_name    
        params['TableName'] = table_name
        if token:
            params['NextToken'] = token
        
        return retry.try_with_backoff({}, self.__client.get_partitions, **params)         

    def get_tables(self, db_name, expression, token):      
        params = dict({})     
        params['DatabaseName'] = db_name            
        params['Expression'] = expression   
        if token:
            params['NextToken'] = token
        
        return  self.__client.get_tables(**params)   

    def delete_partitions(self, db_name, table_name, values):
        if len(values) == 0:
            return None
        params = dict({})     
        params['DatabaseName'] = db_name    
        params['TableName'] = table_name
        params['PartitionsToDelete'] = values       
        print params 
        return retry.try_with_backoff({}, self.__client.batch_delete_partition, **params)   
        
    def __wait_for_crawler(self, name):
        max_attempts = 60
        attempt = 1
        while attempt < max_attempts:
            response = self.get_crawler(name)
            state = response['Crawler']['State']
            print name, 'is', state
            if state == "READY":                
                break
            else:
                attempt += 1
                time.sleep(10)

    def __table_params(self, db_name, table_name, schema, partitions, s3_path):
        params = dict({})     
        params['DatabaseName'] = db_name    
        params['TableInput'] = {
            'Name': table_name,
            'StorageDescriptor': {
                'Columns': schema,
                'Location': s3_path,
                'Parameters': {                    
                    'compressionType':'none',                    
                    'classification':'parquet',                    
                    'typeOfData':'file'
                },
                'SerdeInfo':{
                    'SerializationLibrary':'org.apache.hadoop.hive.ql.io.parquet.serde.ParquetHiveSerDe',
                    'Parameters':{
                            'serialization.format':'1'
                        }
                }
            },
            'Parameters': {                    
                    'compressionType':'none',                    
                    'classification':'parquet',                    
                    'typeOfData':'file'
                },
            'PartitionKeys': partitions
        }
        return params

    def get_table(self, db_name, table_name):
        params = dict({})     
        params['DatabaseName'] = db_name    
        params['Name'] = table_name
        return self.__client.get_table(**params)




    
                
    