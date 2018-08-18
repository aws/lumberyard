#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import boto3

from cgf_utils import aws_utils
from cgf_utils import custom_resource_response

swf_client = aws_utils.ClientWrapper(boto3.client('swf', region_name=aws_utils.current_region))

def handler(event, context):
    request_type = event['RequestType']
    logical_resource_id = event['LogicalResourceId']
    properties = event['ResourceProperties']
    physical_resource_id = aws_utils.get_stack_name_from_stack_arn(event['StackId']) + "-" + logical_resource_id
    domain_name = physical_resource_id + "-domain"
    workflow_type_name = physical_resource_id + "-workflow-type"
    
    if request_type == "Create":
        swf_client.register_domain(
            name=domain_name,
            workflowExecutionRetentionPeriodInDays="15"
        )
        
        swf_client.register_workflow_type(
            domain=domain_name,
            name=workflow_type_name,
            version="1.0",
            defaultTaskStartToCloseTimeout = properties['TaskStartToCloseTimeout'],
            defaultExecutionStartToCloseTimeout = properties['ExecutionStartToCloseTimeout'],
            defaultTaskList = properties['TaskList'],
            defaultChildPolicy = properties['ChildPolicy']
        )
        
        for activity_type in properties['ActivityTypes']:
            swf_client.register_activity_type(
                domain=domain_name,
                name=activity_type,
                version="1.0"
            )
    
    elif request_type == "Update":
        existing_types = set()
        params = {
            'domain': domain_name,
            'registrationStatus': "REGISTERED"
        }
        
        while True:
            response = swf_client.list_activity_types(**params)
            existing_types.update(set([item['activityType']['name'] for item in response['typeInfos']]))
            
            if 'nextPageToken' in response:
                params['nextPageToken'] = response['nextPageToken']
            else:
                break
                
        for activity_type in properties['ActivityTypes']:
            if activity_type in existing_types:
                existing_types.discard(activity_type)
            else:
                swf_client.register_activity_type(
                    domain=domain_name,
                    name=activity_type,
                    version="1.0"
                )
        
        for activity_type in existing_types:
            swf_client.deprecate_activity_type(
                domain=domain_name,
                activityType={
                    'name': activity_type,
                    'version': "1.0"
                }
            )

    elif request_type == "Delete":
        swf_client.deprecate_domain(
            name=domain_name
        )
    
    outputs = {
        'DomainName': domain_name,
        'WorkflowTypeName': workflow_type_name,
        'ActivityTypes': properties['ActivityTypes'],
        'TaskList': properties['TaskList']['name']
    }

    return custom_resource_response.success_response(outputs, physical_resource_id)
