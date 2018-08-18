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
    elif type == 'N':
        result = int(data)
    elif type != 'NULL':
        result = data

    return result

    
def _process_log_line(line):
    pieces = line.split(" ", 1)
    return json.loads(pieces[1])
    
@service.api
def get(request, run_id, time):
    if time >= buffer_time:
        time -= buffer_time
    
    timestamp = datetime.datetime.utcfromtimestamp(time)
    
    if len(run_id) > 1:
        run_key = json.dumps(json.loads(run_id))
    else:
        run_key = run_id
    
    # Load the most recent events
    query_params = {
        'TableName': log_db,
        'ConsistentRead': True,
        'ScanIndexForward': True,
        'Select': "ALL_ATTRIBUTES",
        'KeyConditions': {
            'run-id': {
                'AttributeValueList': [
                    {
                        'S': run_key
                    }
                ],
                'ComparisonOperator': "EQ"
            },
            'event-id': {
                'AttributeValueList': [
                    {
                        'S': timestamp.isoformat()[:19]
                    }
                ],
                'ComparisonOperator': "GE"
            }
        }
    }
    
    events = []
    
    while True:
        response = dynamo_client.query(**query_params)
        events.extend([_process_data({'M': item}) for item in response['Items']])
        
        next_key = response.get('LastEvaluatedKey', None)
        if next_key:
            query_params['ExclusiveStartKey'] = next_key
        else:
            break
            
    result = {
        'run-id': run_id,
        'events': events
    }
    
    return result
    