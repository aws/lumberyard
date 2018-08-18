from __future__ import print_function

import boto3
import datetime
import json

import CloudCanvas

import service

from cgf_utils import aws_utils

# import errors
#
# raise errors.ClientError(message) - results in HTTP 400 response with message
# raise errors.ForbiddenRequestError(message) - results in 403 response with message
# raise errors.NotFoundError(message) - results in HTTP 404 response with message
#
# Any other exception results in HTTP 500 with a generic internal service error message.

log_db = CloudCanvas.get_setting('LogDB')
dynamo_client = boto3.client('dynamodb')

kvs_table = CloudCanvas.get_setting('KVS')
active_workflow_key = 'active_workflow'

@service.api
def post(request):
    # Scan through and delete all existing event records
    existing_records = []
    scan_params = {
        'TableName': log_db,
        'AttributesToGet': ["run-id", "event-id"],
        'Select': "SPECIFIC_ATTRIBUTES"
    }
    
    while True:
        response = dynamo_client.scan(**scan_params)
        existing_records.extend(response['Items'])
        
        next_key = response.get('LastEvaluatedKey', None)
        if next_key:
            scan_params['ExclusiveStartKey'] = next_key
        else:
            break
    
    delete_request_items = [{'DeleteRequest': {'Key': record}} for record in existing_records]
    while len(delete_request_items):
        count = min(len(delete_request_items), 24)
        dynamo_client.batch_write_item(
            RequestItems={log_db: delete_request_items[:count]}
        )
        del delete_request_items[:count]
    
    # Remove the reference to the active workflow
    dynamo_client.delete_item(
        TableName=kvs_table,
        Key={
            'key': {'S': active_workflow_key}
        }
    )
    
    return True
