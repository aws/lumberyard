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

import importlib
import boto3
import CloudCanvas
import service
from datetime import datetime

staging_table_name = CloudCanvas.get_setting('StagingTable')

staging_table = boto3.resource('dynamodb').Table(staging_table_name)

content_bucket_name = CloudCanvas.get_setting('StagingBucket')

def get_presigned_url_lifetime():
    return 100
    
@service.api 
def request_url_list(request, request_content = None):

    print 'querying status of {}'.format(request_content)
    print 'Content bucket is ' + content_bucket_name
    
    result = 'What i heard was {}'.format(request_content)
    file_list = request_content.get('FileList')
    
    resultList = []
    if file_list is None:
        print 'Request was empty'
        raise errors.ClientError('Invalid Request (Empty)')
    else: 
        for file_name in file_list:
            _add_file_response(resultList, file_name)
    
    return { 'ResultList' : resultList }
    
def _get_time_format():
    return '%b %d %Y %H:%M'
    
def get_formatted_time_string(timeval):
    return datetime.strftime(timeval, _get_time_format())
 
def get_struct_time(timestring):
    try:
        return datetime.strptime(timestring, _get_time_format())
    except ValueError:
        raise HandledError('Expected time format {}'.format(get_formatted_time_string(datetime.utcnow())))
        
def _get_formatted_time(timeval):
    return datetime.strftime(timeval, '%b %d %Y %H:%M')
    
def _add_file_response(resultList, file_name):
    print 'file name requested is {}'.format(file_name)

    resultData = {}
    resultData['FileName'] = file_name
    resultList.append(resultData)
    
    response_data = staging_table.get_item(
	    Key={
            'FileName': file_name
	    })

		
    if response_data is None:
        resultData['FileStatus'] = 'Invalid File'
        return
	
    item_data = response_data.get('Item', None)
    
    if item_data is None:
        resultData['FileStatus'] = 'Invalid File (No Item)'
        return
        
    print 'item data was {}'.format(item_data)
    
    staging_status = item_data.get('StagingStatus','UNKNOWN')
    print 'Staging status is {}'.format(staging_status)
    
    ## We'll have more designations later - currently PUBLIC or WINDOW are required before any more checks can be processed
    if staging_status not in['PUBLIC', 'WINDOW']:
        print 'File is not currently staged'
        resultData['FileStatus'] = 'Data is unavailable'  
        return

    current_time = datetime.utcnow()
    print 'Current time is ' + _get_formatted_time(current_time)
    
    staging_start = item_data.get('StagingStart')
    staging_end = item_data.get('StagingEnd')

    if staging_status == 'WINDOW':
        if staging_start == None and staging_end == None:
            print 'Item is staged as WINDOW with no start or end - treating as unavailable'
            resultData['FileStatus'] = 'Data is unavailable' 
            return
            
        print 'staging_start is {}'.format(staging_start)

        if staging_start != None:
            startdate = get_struct_time(staging_start)
        
            if startdate > current_time:
                print 'Start date is in the future, content not ready'
                resultData['FileStatus'] = 'Not available yet'
                return

        print 'staging_end is {}'.format(staging_end)

        if staging_end != None:
            print 'staging_end is ' + staging_end
            enddate = get_struct_time(staging_end)

            if enddate < current_time:
                print 'End date is in the past, content no longer available'
                resultData['FileStatus'] = 'No longer available'
                return
            
    s3Client = boto3.client('s3')
    returnUrl = s3Client.generate_presigned_url('get_object', Params = {'Bucket': content_bucket_name, 'Key': file_name}, ExpiresIn = get_presigned_url_lifetime())
       
    print 'Returned url is ' + returnUrl
    resultData['PresignedURL'] = returnUrl
    
    signature = item_data.get('Signature','')
    print 'Returning Signature {}'.format(signature)
    resultData['Signature'] = signature

        
 
