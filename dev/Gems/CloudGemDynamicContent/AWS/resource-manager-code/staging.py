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
# $Revision: #1 $

from resource_manager.errors import HandledError
from cgf_utils import custom_resource_utils
import resource_manager.util
import boto3
import json
import dynamic_content_settings
import show_manifest
import content_manifest
import datetime

def _get_staging_table(context, deployment_name):
    return _get_staging_table_by_name(context, deployment_name, dynamic_content_settings.get_default_resource_group(), dynamic_content_settings.get_default_staging_table_name())

def _get_staging_table_by_name(context, deployment_name, resource_group_name = dynamic_content_settings.get_default_resource_group(), table_name = dynamic_content_settings.get_default_staging_table_name()):
    '''Returns the resource id of the staging table.'''
    return _get_stack_resource_by_name(context, deployment_name, resource_group_name, table_name)
 
def _get_stack_resource_by_name(context, deployment_name, resource_group_name, resource_name):
    '''Returns the resource id of the staging table.'''
    if deployment_name is None:
        deployment_name = context.config.default_deployment
  
    stack_id = context.config.get_resource_group_stack_id(deployment_name , resource_group_name, optional=True)

    resource_obj = context.stack.get_physical_resource_id(stack_id, resource_name)
    resource_arn = custom_resource_utils.get_embedded_physical_id(resource_obj)
    show_manifest.found_stack(resource_arn)
    return resource_arn
    
def _get_request_lambda(context):
    return _get_request_lambda_by_name(context, context.config.default_deployment, dynamic_content_settings.get_default_resource_group(), dynamic_content_settings.get_default_request_lambda_name())

def _get_request_lambda_by_name(context, deployment_name, resource_group_name = dynamic_content_settings.get_default_resource_group(), lambda_name = dynamic_content_settings.get_default_request_lambda_name()):
    '''Returns the resource id of the request lambda.'''
    return _get_stack_resource_by_name(context, deployment_name, resource_group_name, lambda_name)
    
def _get_time_format():
    return '%b %d %Y %H:%M'
    
def get_formatted_time_string(timeval):
    return datetime.datetime.strftime(timeval, _get_time_format())

def get_struct_time(timestring):
    try:
        return datetime.datetime.strptime(timestring, _get_time_format())
    except ValueError:
        raise HandledError('Expected time format {}'.format(get_formatted_time_string(datetime.datetime.utcnow())))
    
def parse_staging_arguments(args):
    staging_args = {}
    staging_args['StagingStatus'] = args.staging_status
    staging_args['StagingStart'] = args.start_date 
    staging_args['StagingEnd'] = args.end_date

    return staging_args
          
def command_set_staging_status(context, args):
    staging_args = parse_staging_arguments(args)
    
    set_staging_status(args.file_path, context, staging_args, context.config.default_deployment)  
    
def set_staging_status(file_path, context, staging_args, deployment_name):

    staging_status = staging_args.get('StagingStatus')
    staging_start = staging_args.get('StagingStart')
    staging_end = staging_args.get('StagingEnd')
    signature = staging_args.get('Signature')
    parentPak = staging_args.get('Parent')
    fileSize = staging_args.get('Size')
    fileHash = staging_args.get('Hash')

    if not staging_status:
        staging_status = 'PRIVATE'
        
    if staging_status == 'WINDOW':
        if not staging_start:
            raise HandledError('No start-date set for WINDOW staging_status')
            
        if not staging_end:
            raise HandledError('No end-date set for WINDOW staging_status')  
          
    dynamoDB = context.aws.client('dynamodb', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    
    table_arn = _get_staging_table(context, deployment_name)
    
    response = {}
    attributeUpdate = {
        'StagingStatus' : {
            'Value': {
                'S': staging_status
            }
        }
    }
    
    if len(parentPak):
        attributeUpdate['Parent'] = {
            'Value': {
                'S': parentPak
            }
        }
        
    if signature:
        attributeUpdate['Signature'] = {
            'Value': {
                'S': signature
            }
        }
    else:
        attributeUpdate['Signature'] = {
            'Action': 'DELETE'
        }

    if fileSize is not None:
        attributeUpdate['Size'] = {
            'Value': {
                'S': str(fileSize)
            }
        }
    else:
        attributeUpdate['Size'] = {
            'Action': 'DELETE'
        }

    if fileHash is not None and len(fileHash) > 0:
        attributeUpdate['Hash'] = {
            'Value': {
                'S': str(fileHash)
            }
        }
    else:
        attributeUpdate['Hash'] = {
            'Action': 'DELETE'
        }

    if staging_status == 'WINDOW':
        time_start = None
        if staging_start.lower() == 'now':
            time_start = get_formatted_time_string(datetime.datetime.utcnow())
        else:
            # Validate our input
            time_start = get_formatted_time_string(get_struct_time(staging_start))
            
        time_end = None
        if staging_end.lower() == 'never':
            time_end = get_formatted_time_string(datetime.datetime.utcnow() + datetime.timedelta(days=dynamic_content_settings.get_max_days()))
        else:
            # Validate our input
            time_end = get_formatted_time_string(get_struct_time(staging_end))
            
        attributeUpdate['StagingStart'] = {
            'Value': {
                'S': time_start
            }
        }
        
        attributeUpdate['StagingEnd'] = {
            'Value': {
                'S': time_end
            }
        }
            
    try:
        response = dynamoDB.update_item(
            TableName=table_arn,
            Key={
                'FileName': {
                   'S': file_path
                }
            },
            AttributeUpdates= attributeUpdate,
            ReturnValues='ALL_NEW'
        )
    except Exception as e:
        raise HandledError(
            'Could not update status for'.format(
                FilePath = file_path,
            ),
            e
        )
    item_data = response.get('Attributes', None)
    
    new_staging_status = {}
    new_staging_status['StagingStatus'] = item_data.get('StagingStatus',{}).get('S','UNSET')
    new_staging_status['StagingStart'] = item_data.get('StagingStart',{}).get('S')
    new_staging_status['StagingEnd'] = item_data.get('StagingEnd',{}).get('S')  
    new_staging_status['Parent'] = item_data.get('Parent',{}).get('S')  
    
    show_manifest.show_staging_status(file_path, new_staging_status)

def empty_staging_table(context):
    dynamoDB = context.aws.client('dynamodb', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    
    table_arn = _get_staging_table(context, context.config.default_deployment)
    response = dynamoDB.scan(
        TableName=table_arn
    )
    items = response['Items']
    
    for this_item in items:
        key_path = this_item.get('FileName')
        
        if key_path is not None:
            remove_entry(context, key_path)
            
def remove_entry(context, file_path):
    dynamoDB = context.aws.client('dynamodb', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    
    table_arn = _get_staging_table(context, context.config.default_deployment)
    try:
        response = dynamoDB.delete_item(
            TableName=table_arn,
            Key={
                'FileName': file_path
            }
        )
    except Exception as e:
        raise HandledError(
            'Could not delete entry for for'.format(
                FilePath = file_path,
            ),
            e
        )

def command_request_url(context, args):
    file_name = args.file_path
    
    file_list = []
    file_list.append(file_name)
    
    request = {}
    request['FileList'] = file_list
    
    lambdaClient = context.aws.client('lambda', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    
    lambda_arn = _get_request_lambda(context)
    
    contextRequest = json.dumps(request)
    print 'Using request {}'.format(contextRequest)
    try:
        response = lambdaClient.invoke(
            FunctionName=lambda_arn,
            InvocationType='RequestResponse',
            Payload=contextRequest
        )
        payloadstream = response.get('Payload',{})
        payload_string = payloadstream.read()
        print 'Got result {}'.format(payload_string)
    except Exception as e:
        raise HandledError(
            'Could not get URL for'.format(
                FilePath = file_name,
            ),
            e
        )

def signing_status_changed(context, key, do_signing):
    dynamoDB = context.aws.client('dynamodb', region=resource_manager.util.get_region_from_arn(context.config.project_stack_id))
    table_arn = _get_staging_table(context, context.config.default_deployment)

    try:
        response = dynamoDB.get_item(
            TableName = table_arn,
            Key = {'FileName': {'S' : key}}
        )

    except Exception as e:
        if e.response['Error']['Code'] == 'ResourceNotFoundException':
            return True
        else:
            raise HandledError(
                'Could not get signing status for {}'.format(
                    key,
                ),
                e
            )

    pak_is_signed = response.get('Item', {}).get('Signature', {}).get('S', '') != ''
    return pak_is_signed != do_signing

