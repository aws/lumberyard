from __future__ import print_function

import uuid

import boto3
import botocore.exceptions

import CloudCanvas

table_name = CloudCanvas.get_setting('Table')

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table(table_name)

CONDITION_KEY_EXISTS = boto3.dynamodb.conditions.Attr('Key').exists()
CONDITION_KEY_DOES_NOT_EXIST = boto3.dynamodb.conditions.Not(CONDITION_KEY_EXISTS)


def list(exclusive_start = None, count = None):

    print('table_data.list started', exclusive_start, count)
    print('using table', table.name)
    
    args = {
        # Key is a reserved word and can't be used in the projection expression.
        # See: http://docs.aws.amazon.com/amazondynamodb/latest/developerguide/Expressions.ExpressionAttributeNames.html#Expressions.ExpressionAttributeNames.ReservedWords
        'ProjectionExpression': '#key',
        'ExpressionAttributeNames': { '#key': 'Key' }
    }

    if exclusive_start:
        args['ExclusiveStartKey'] = { 'Key': exclusive_start }

    if count:
        args['Limit'] = count

    res = table.scan(**args)

    keys = [ item['Key'] for item in res['Items'] ]
    next = res.get('LastEvaluatedKey', {}).get('Key', None)
    
    print('table_data.list succeeded', keys, next)
    return keys, next 


def create(data):
    
    print('table_data.create started', data)
    print('using table', table.name)
    
    key = str(uuid.uuid4())
    print('table_data.create generated key', key)

    item = data_to_item(key, data)

    table.put_item( Item = item, ConditionExpression = CONDITION_KEY_DOES_NOT_EXIST)
        
    print('bucket_create.create succeeded', key)
    return key


def read(key):

    print('table_data.read started', key)
    print('using table', table.name)
    
    res = table.get_item( Key = { 'Key': key } )
    item = res.get('Item', None)

    if item:
        return item_to_data(item)
    else:
        return None


def update(key, data):

    print('table_data.update started', key, data)
    print('using table', table.name)
    
    item = data_to_item(key, data)

    try:
        table.put_item( Item = item, ConditionExpression = CONDITION_KEY_EXISTS)
    except botocore.exceptions.ClientError as e:
        if e.response['Error']['Code'] == 'ConditionalCheckFailedException':
            return False
        else:
            raise

    print('table_data.update succeeded')
    return True


def delete(key):

    print('table_data.delete started', key)
    print('using table', table.name)

    try:
        table.delete_item( Key = { 'Key': key }, ConditionExpression = CONDITION_KEY_EXISTS)
    except botocore.exceptions.ClientError as e:
        if e.response['Error']['Code'] == 'ConditionalCheckFailedException':
            return False
        else:
            raise

    print('table_data.delete succeeded')
    return True



def data_to_item(key, data):
    return {
        'Key': key,
        'ExamplePropertyA': data['ExamplePropertyA'],
        'ExamplePropertyB': data['ExamplePropertyB']
    }


def item_to_data(item):
    return {
        'ExamplePropertyA': item['ExamplePropertyA'],
        'ExamplePropertyB': item['ExamplePropertyB']
    }        

