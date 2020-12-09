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
import os
import boto3
import CloudCanvas
import service
from datetime import datetime
from .cloudfront_request import cdn_name, get_cdn_presigned

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import padding

from botocore.client import Config


staging_table_name = CloudCanvas.get_setting('StagingTable')

staging_table = boto3.resource('dynamodb').Table(staging_table_name)

content_bucket_name = CloudCanvas.get_setting('StagingBucket')

def get_presigned_url_lifetime():
    return 100
    
@service.api 
def request_url_list(request, request_content = None):

    print('querying status of {}'.format(request_content))
    print('Content bucket is {}'.format(content_bucket_name))
    
    file_list = request_content.get('FileList')
    # ManifestData flag means to include data found in the manifest in the
    # response (Size, Hash) so the manifest request can be skipped
    manifest_data = request_content.get('ManifestData', False)
    resultList = []
    if file_list is None:
        print('Request was empty')
        raise errors.ClientError('Invalid Request (Empty)')
    else: 
        for file_entry in file_list:
            _add_file_response(resultList, file_entry.get('FileName'), manifest_data, file_entry.get('FileVersion'))
    
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

def _get_s3_presigned(file_name, version_id=None):
    print('Getting presigned url for {} from s3'.format(file_name))

    s3Client = _get_s3_client()
    params = {'Bucket': content_bucket_name, 'Key': file_name}
    if version_id:
        params['VersionId'] = version_id

    return s3Client.generate_presigned_url('get_object', Params = params, ExpiresIn = get_presigned_url_lifetime())

def _get_presigned_url(file_name, version_id=None):
    if cdn_name:
        return get_cdn_presigned(file_name, version_id)
    else:
        return _get_s3_presigned(file_name, version_id)

def _add_file_response(resultList, file_name, manifest_data, version_id=None):
    if not file_name:
        print('Invalid File Name (None)')
        return
    print('file name requested is {}'.format(file_name))

    resultData = {}
    resultData['FileName'] = file_name
    resultList.append(resultData)

    item_versions = []
    if version_id:
        response_data = staging_table.get_item(
	        Key={
                'FileName': file_name, 
                'VersionId': version_id
            }
        )
	
        item_data = response_data.get('Item', None)
        item_versions.append(item_data)
    else:
        # Query the staging settings table by the file name to retrieve all the available versions.
        response_data = staging_table.query(
            ExpressionAttributeNames={
                "#FileName": "FileName"
            },
            ExpressionAttributeValues={
                ':filenameval': file_name
            },
            KeyConditionExpression='#FileName = :filenameval'
        )

        if response_data is None:
            resultData['FileStatus'] = 'Invalid File'
            return
        item_versions = response_data.get('Items', [])
    
    active_versions = []
    # Iterate through all the available versions to find the active one
    for item_data in item_versions: 
        if item_data is None:
            print('Invalid File (No Item)')
            continue

        print('item data was {}'.format(item_data))

        staging_status = item_data.get('StagingStatus','UNKNOWN')
        print('Staging status is {}'.format(staging_status))
    
        ## We'll have more designations later - currently PUBLIC or WINDOW are required before any more checks can be processed
        if staging_status not in['PUBLIC', 'WINDOW']:
            print('File is not currently staged')
            continue

        current_time = datetime.utcnow()
        print('Current time is {}'.format(_get_formatted_time(current_time)))
    
        staging_start = item_data.get('StagingStart')
        staging_end = item_data.get('StagingEnd')

        if staging_status == 'WINDOW':
            if staging_start == None and staging_end == None:
                print('Item is staged as WINDOW with no start or end - treating as unavailable')
                continue
            
            print('staging_start is {}'.format(staging_start))

            if staging_start != None:
                startdate = get_struct_time(staging_start)
        
                if startdate > current_time:
                    print('Start date is in the future, content not ready')
                    continue

            if staging_end != None:
                print('staging_end is {}'.format(staging_end))
                enddate = get_struct_time(staging_end)

                if enddate < current_time:
                    print('End date is in the past, content no longer available')
                    continue
            else:
                print('staging_end is None')

        active_versions.append(item_data)
    
    num_active_versions = len(active_versions)
    if num_active_versions == 0:
        resultData['FileStatus'] = 'Data is unavailable'
        return
    elif num_active_versions > 1:
        # It's legal for more than one version to be "available" as long as customers make a versioned request
        # This is specifically to catch the case where customers didn't provide a version and just want to get the active version
        print('Different versions of a same file are active at the same time.')
        resultData['FileStatus'] = 'Data is conflict'
        return

    returnUrl = _get_presigned_url(file_name, active_versions[0].get('VersionId'))
       
    print('Returned url is {}'.format(returnUrl))
    resultData['PresignedURL'] = returnUrl
    
    signature = active_versions[0].get('Signature','')
    print('Returning Signature {}'.format(signature))
    resultData['Signature'] = signature

    ## include optional "Manifest Data" to allow for requests of known bundles without using the manifest
    if manifest_data:
        ## Empty string in the case of an unknown size
        resultData['Size'] = active_versions[0].get('Size', '')
        resultData['Hash'] = active_versions[0].get('Hash', '')

def _get_s3_client():
    '''Returns a S3 client with proper configuration.'''
    if not hasattr(_get_s3_client, 'client'):
        current_region = os.environ.get('AWS_REGION')
        if current_region is None:
            raise RuntimeError('AWS region is empty')

        configuration = Config(signature_version='s3v4', s3={'addressing_style': 'path'})
        _get_s3_client.client = boto3.client('s3', region_name=current_region, config=configuration)

    return _get_s3_client.client