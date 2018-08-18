from __future__ import print_function

import boto3
import datetime
import json

import CloudCanvas

import service

# import errors
#
# raise errors.ClientError(message) - results in HTTP 400 response with message
# raise errors.ForbiddenRequestError(message) - results in 403 response with message
# raise errors.NotFoundError(message) - results in HTTP 404 response with message
#
# Any other exception results in HTTP 500 with a generic internal service error message.

log_db = CloudCanvas.get_setting('LogDB')
dynamo_client = boto3.client('dynamodb')

buffer_time = 5


def _process_data(d):
    result = None
    type, data = list(d.items())[0]
    if type == 'L':
        result = [_process_data(item) for item in data]
    elif type == 'M':
        result = {k: _process_data(v) for k, v in data.iteritems()}
    elif type != 'NULL':
        result = data

    return result

    
def _process_log_line(line):
    pieces = line.split(" ", 1)
    return json.loads(pieces[1])
    
@service.api
def get(request):
    scan_params = {
        'TableName': log_db,
        'AttributesToGet': ["run-id", "timestamp"],
        'Select': "SPECIFIC_ATTRIBUTES"
    }
    
    build_ids = {}
    
    while True:
        response = dynamo_client.scan(**scan_params)
        for item in response['Items']:
            build_id = item['run-id']['S']
            timestamp = item['timestamp']['S']
            build_ids[build_id] = max(timestamp, build_ids.get(build_id, ""))
        
        next_key = response.get('LastEvaluatedKey', None)
        if next_key:
            scan_params['ExclusiveStartKey'] = next_key
        else:
            break
    
    result = list(build_ids.items())
    result.sort(key=lambda x: x[1], reverse=True)
    result = [{'id': id, 'start': start} for id, start in result]
    
    return result
    