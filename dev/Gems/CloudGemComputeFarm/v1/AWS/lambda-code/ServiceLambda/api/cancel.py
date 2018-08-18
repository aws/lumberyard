from __future__ import print_function

import boto3
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

workflow = CloudCanvas.get_setting('Workflow')
workflow_domain_name = workflow + '-domain'

swf_client = boto3.client('swf', region_name=aws_utils.current_region)


@service.api
def post(request, workflow_id):
    response = swf_client.terminate_workflow_execution(
        domain=workflow_domain_name,
        workflowId=workflow_id
    )
    
    return "success"
