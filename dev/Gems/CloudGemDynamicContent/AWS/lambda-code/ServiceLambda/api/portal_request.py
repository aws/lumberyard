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
import CloudCanvas
import service
import errors
import json
from botocore.exceptions import ClientError
from botocore.client import Config
import cgf_lambda_settings
import cgf_service_client
from datetime import datetime

def __do_communicator_updates():
    # Set to false if updates should not be sent at all, even if the web communicator is enabled
    return True
    
def _get_content_bucket():
    if not hasattr(_get_content_bucket,'content_bucket'):
        content_bucket_name = CloudCanvas.get_setting('StagingBucket')   
    
        print 'Bucket name is {}'.format(content_bucket_name)
    
        print 'attempting to get S3 resource from local region'
        bucketResource = boto3.resource('s3', config=Config(signature_version='s3v4'))
        _get_content_bucket.content_bucket = bucketResource.Bucket(content_bucket_name)
    
        if _get_content_bucket.content_bucket is None:
            raise errors.PortalRequestError('No Content Bucket')

    return _get_content_bucket.content_bucket
    
def _get_staging_table():
    if not hasattr(_get_staging_table,'staging_table'):
        staging_table_name = CloudCanvas.get_setting('StagingTable')   
    
        print 'Table name is {}'.format(staging_table_name)
    
        print 'attempting to get dynamodb resource from local region'
        dynamoresource = boto3.resource('dynamodb')
        _get_staging_table.staging_table = dynamoresource.Table(staging_table_name)
    
        if _get_staging_table.staging_table is None:
            raise errors.PortalRequestError('No Staging Table')

    return _get_staging_table.staging_table
    
@service.api 
def list_all_content(request):
    print 'Listing portal content'
    resultData = []
       
    tableinfo = _get_staging_table()
    table_data = tableinfo.scan()
    
    this_result = {}
    for item_data in table_data['Items']:
        print 'Found item {}'.format(item_data)
        resultData.append(item_data)
        
    while table_data.get('LastEvaluatedKey'):
        table_data = tableinfo.scan(ExclusiveStartKey=table_data.get('LastEvaluatedKey'))

        this_result = {}
        for item_data in table_data['Items']:
            print 'Found item {}'.format(item_data)
            resultData.append(item_data)   
            
    return { 'PortalFileInfoList' : resultData}

@service.api 
def delete_all_content(request):
    print 'Deleting portal content'
    results_list = []
    _empty_bucket_contents()
    _clear_staging_table(results_list)
    return  { 'DeletedFileList' : results_list }
    
def _get_bucket_content_list():
    print 'Getting bucket content list'
    s3 = boto3.client('s3', config=Config(signature_version='s3v4'))
    content_bucket_name = CloudCanvas.get_setting('StagingBucket') 
    nextMarker = 0
    contentsList = []
    while True:
        try:
            print 'Listing objects in {}'.format(content_bucket_name)
            res = s3.list_objects(
                Bucket = content_bucket_name,
                Marker = str(nextMarker)
            )
            thisList = res.get('Contents',[])
            contentsList += thisList
            print 'Appending items: {}'.format(thisList)
            if len(thisList) < get_list_objects_limit():
                break
            nextMarker += get_list_objects_limit()
        except ClientError as e:
            raise errors.PortalRequestError('Failed to list bucket content: {}'.format(e.response['Error']['Message']))

            
    return contentsList

def _send_bucket_delete_list(objectList):
    print 'Attempting to delete list'
    content_bucket = _get_content_bucket()
    try:
        res = content_bucket.delete_objects(
            Delete = { 'Objects': objectList }
        )
    except ClientError as e:
        raise errors.PortalRequestError('Failed to delete list from bucket: {}'.format(e.response['Error']['Message']))
            
def _remove_bucket_item(key_path):
    content_bucket = _get_content_bucket()
    print 'Attempting to delete object {}'.format(key_path)
    try:
        res = content_bucket.delete_objects(
            Delete={
                'Objects': [ 
                    {
                        'Key': key_path
                    }
                ]
            }
        )
        print 'Success: {}'.format(res)
    except ClientError as e:
        raise errors.PortalRequestError('Failed to remove item from bucket: {}'.format(e.response['Error']['Message']))
            
def _empty_bucket_contents():
    contentsList = _get_bucket_content_list()
    listLength = len(contentsList)
    curIndex = 1
    objectList = []
    
    for thisContent in contentsList:
        objectList.append({ 'Key': thisContent.get('Key',{})})
        if len(objectList) == get_list_objects_limit():
            _send_bucket_delete_list(objectList)
            objectList = []
    if len(objectList):
        _send_bucket_delete_list(objectList)

def _clear_staging_table(return_list):
    print 'Clearing staging table'
    staging_table = _get_staging_table()
    response = staging_table.scan()
    items = response['Items']
    
    for this_item in items:
        key_path = this_item.get('FileName')
        
        if key_path is not None:
            if _remove_entry(key_path):
                return_list.append(key_path)
            
def _remove_entry(key_path):
    print 'Removing entry {}'.format(key_path)
    staging_table = _get_staging_table()
    try:
        response = staging_table.delete_item(
             Key={
                'FileName': key_path
            }
        )
        return True
    except ClientError as e:
        raise errors.PortalRequestError('Failed to remove entry from table: {}'.format(e.response['Error']['Message']))

@service.api 
def delete_content(request, file_name):
    print 'Deleting content for file {}'.format(file_name)
    _remove_bucket_item(file_name)
    _remove_entry(file_name)
    return { 'DeletedFileList' : [ file_name ]  }
    
@service.api 
def describe_content(request, file_name):
    print 'Describing portal content'
    resultData = []
    
    staging_table = _get_staging_table()
    
    try:
        response_data = staging_table.get_item(
            Key={
                'FileName': file_name
            })
    except:
        raise errors.ClientError('Failed to retrieve data for ' + file_name)
    
    item_data = response_data.get('Item')
    
    if not item_data:
        raise errors.ClientError('No data for ' + file_name)   
        
    return { 'PortalFileInfo' : item_data}


def __send_communicator_broadcast(message):
    if not __do_communicator_updates():
        return
        
    interface_url = cgf_lambda_settings.get_service_url("CloudGemWebCommunicator_sendmessage_1_0_0")
    if not interface_url:
        print 'Messaging interface not found'
        return
        
    client = cgf_service_client.for_url(interface_url, verbose=True, session=boto3._get_default_session())
    result = client.navigate('broadcast').POST({"channel": "CloudGemDynamicContent", "message": message})
    print 'Got send result {}'.format(result)

def __send_data_updated(pak_name, status):
    data = {}
    data['update'] = 'FILE_STATUS_CHANGED'
    data['pak_name'] = pak_name
    data['status'] = status
    
    __send_communicator_broadcast(json.dumps(data))
    
@service.api 
def set_file_statuses(request, request_content = None):
    print 'Setting file status'
    
    staging_table = _get_staging_table()
    
    print 'Got request_content {}'.format(request_content)
    resultData = [] 
    file_list = request_content.get('FileList', [])
    print 'file_list is {}'.format(file_list)
    for thisRequest in file_list:
        print 'Processing request {}'.format(thisRequest)
        fileName = thisRequest.get('FileName')
        if fileName is None:
            print 'No FileName specified'
            continue
            
        print 'Got request for file {} status {} start {} end {}'.format(fileName, thisRequest.get('StagingStatus'), thisRequest.get('StartDate'), thisRequest.get('EndDate'))
                
        attribute_updates = ''
        expression_attribute_values = {}
        new_status = thisRequest.get('StagingStatus')
        if new_status is not None:
            attribute_updates += ' StagingStatus = :status'
            expression_attribute_values[':status'] = new_status         
            
        if 'StagingStart' in thisRequest:
            start_date = thisRequest.get('StagingStart')
            if len(attribute_updates):
                attribute_updates += ','
            attribute_updates += ' StagingStart = :start'
            expression_attribute_values[':start'] = start_date
            
        if 'StagingEnd' in thisRequest:
            end_date = thisRequest.get('StagingEnd')
            if len(attribute_updates):
                attribute_updates += ','
            attribute_updates += ' StagingEnd = :end'
            expression_attribute_values[':end'] = end_date
          
        update_expression = '{} {}'.format('SET',attribute_updates)
        try:
            response_info = staging_table.update_item(Key={ 'FileName': fileName},UpdateExpression=update_expression,ExpressionAttributeValues=expression_attribute_values,ReturnValues='ALL_NEW')
        except ClientError as e:
            print(e.response['Error']['Message'])
            continue
        
        returned_item = response_info.get('Attributes',{})
        
        resultData.append(returned_item)
        
        __send_data_updated(fileName, new_status)
    
    return { 'PortalFileInfoList' : resultData}
    
def get_list_objects_limit():
    return 1000 # This is an AWS internal limit on list_objects

