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

    
@service.api
def post(request, body):
    workflow_id = body['workflow_id']
    run_params = body['run_params']

    if workflow_id is None or len(workflow_id) == 0:
        now = datetime.datetime.utcnow().replace(microsecond=0)
        workflow_id = "exec-%s" % now.isoformat().replace(":", ".")

    try:
        response = swf_client.start_workflow_execution(
            domain=workflow_domain_name,
            workflowId=workflow_id,
            workflowType={
                'name': workflow_type_name,
                'version': "1.0"
            },
            input=run_params
        )

        response = {
            'workflowId': workflow_id,
            'runId': response['runId']
        }
       
        dynamo_client.put_item(
            TableName=kvs_table,
            Item={
                'key': {'S': active_workflow_key},
                'value': {'S': json.dumps(response)}
            }
        )
        
    except ClientError as e:
        if e.response['Error']['Code'] == "WorkflowExecutionAlreadyStartedFault":
            raise errors.ClientError("A workflow execution with this name already exists")
        else:
            raise e
    
    return json.dumps(response)
