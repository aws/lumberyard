from __future__ import print_function

import json
import uuid

import boto3
import botocore.exceptions
from botocore.client import Config
import CloudCanvas

bucket_name = CloudCanvas.get_setting('Bucket')

s3 = boto3.resource('s3', config=Config(signature_version='s3v4'))
bucket = s3.Bucket(bucket_name)


def list(start = None, count = None):

    print('bucket_data.list started', start, count)
    print('using bucket', bucket.name)
    
    args = {}
    if start:
        args['Marker'] = start
        
    summaries = bucket.objects.filter(**args)
    if count:
        summaries = summaries.limit(count)
        
    result = [ summary.key for summary in summaries ]
    
    print('bucket_data.list succeeded', result)
    return result


def create(data):
    
    print('bucket_data.create started', data)
    print('using bucket', bucket.name)
    
    key = str(uuid.uuid4())
    print('bucket_data.create generated key', key)
    
    object = bucket.Object(key)
    object.put(Body=json.dumps(data))
    
    print('bucket_create.create succeeded', key)
    return key


def read(key):

    print('bucket_data.read started', key)
    print('using bucket', bucket.name)
    
    object = bucket.Object(key)
    try:
    
        content = object.get()['Body'].read()
        print('bucket_data.read content', content)
        
        result = json.loads(content)
        print('bucket_data.read json', result)
        
        return result
    
    except botocore.exceptions.ClientError as e:
    
        if e.response.get('Error', {}).get('Code') == 'NoSuchKey':
        
            print('bucket_data.read failed: data not found')
            return None
            
        else:
        
            print('bucket_data.read failed:', e)
            raise    


def update(key, data):

    print('bucket_data.update started', key, data)
    print('using bucket', bucket.name)
    
    object = bucket.Object(key)
    try:
    
        object.load() # verify it exists
        
    except botocore.exceptions.ClientError as e:
    
        if e.response.get('Error', {}).get('Code') == '404':
            
            print('bucket_data.update failed: data not found')
            return False
            
        else:
            
            print('bucket_data.update failed:', e)
            raise    
            
    object.put(Body=json.dumps(data))
    
    print('bucket_data.update succeeded')
    return True


def delete(key):

    print('bucket_data.delete started', key)
    print('using bucket', bucket.name)
    
    object = bucket.Object(key)
    try:
    
        object.delete()
        print('bucket_data.delete succeeded')
        return True
        
    except botocore.exceptions.ClientError as e:
        
        if  e.response.get('Error', {}).get('Code') == 'NoSuchKey':
        
            print('bucket_data.delete failed: data not found')
            return False
            
        else:
        
            print('bucket_data.delete failed:', e)
            raise