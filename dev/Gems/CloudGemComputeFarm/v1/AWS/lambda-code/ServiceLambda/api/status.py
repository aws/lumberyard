from __future__ import print_function

import boto3
import datetime
import json

from botocore.exceptions import ClientError

import CloudCanvas

import errors
import service

from cgf_utils import aws_utils

import fleet

# import errors
#
# raise errors.ClientError(message) - results in HTTP 400 response with message
# raise errors.ForbiddenRequestError(message) - results in 403 response with message
# raise errors.NotFoundError(message) - results in HTTP 404 response with message
#
# Any other exception results in HTTP 500 with a generic internal service error message.

workflow = CloudCanvas.get_setting('Workflow')
workflow_domain_name = workflow + '-domain'
workflow_type_name = workflow + '-workflow-type'

swf_client = boto3.client('swf', region_name=aws_utils.current_region)
dynamo_client = boto3.client('dynamodb')

kvs_table = CloudCanvas.get_setting('KVS')
active_workflow_key = 'active_workflow'


def _serialize_json_datetime(obj):
    if isinstance(obj, (datetime.datetime, datetime.date)):
        return obj.isoformat()
        
    raise TypeError("Type %s is not serializable" % type(obj))


def _init_active_workflow():
    active_workflow = None

    response = dynamo_client.query(
        TableName=kvs_table,
        ConsistentRead=True,
        Select="ALL_ATTRIBUTES",
        KeyConditions={
            'key': {
                'AttributeValueList': [
                    {'S': active_workflow_key}
                ],
                'ComparisonOperator': "EQ"
            }
        }
    )
    
    if len(response['Items']):
        active_workflow = json.loads(response['Items'][0]['value']['S'])
        
    return active_workflow

    
@service.api
def get(request, workflow_id):
    if not workflow_id or len(workflow_id) == 0:
        workflow_id = _init_active_workflow()
    else:
        workflow_id = json.loads(workflow_id)
    
    result = {}
    
    if workflow_id:
        result = swf_client.describe_workflow_execution(
            domain=workflow_domain_name,
            execution=workflow_id
        )
        
    return json.dumps(result, default=_serialize_json_datetime)
