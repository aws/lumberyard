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

from service import api
from botocore.client import Config
from botocore.exceptions import ClientError
import boto3
import os
import errors
import re

PROJECT_CGP_ROOT_FILE = 'www/index.html'
PROJECT_CGP_APP_FILE = 'www/bundles/app.bundle.js'
PROJECT_CGP_DEPENDENCY_FILE = 'www/bundles/dependencies.bundle.js'
BUCKET_ID = os.environ.get("CloudGemPortalBucket")
REGION = os.environ.get('AWS_REGION')
DEPENDENCY_REGEX_PATTERN = "System\.import\(dependencies\)"
APP_REGEX_PATTERN = "System\.import\(app\)"

@api
def open(request):    
    s3_client = boto3.client('s3', REGION, config=Config(region_name=REGION,signature_version='s3v4', s3={'addressing_style': 'virtual'}))
    
    content = get_index(s3_client)
    content = content.replace(get_match_group(DEPENDENCY_REGEX_PATTERN, content),
                              "System.import('{}')".format(get_presigned_url(s3_client, PROJECT_CGP_DEPENDENCY_FILE)))
    content = content.replace(get_match_group(APP_REGEX_PATTERN, content),
                              "System.import('{}')".format(get_presigned_url(s3_client, PROJECT_CGP_APP_FILE)))        
    print content
    return content
    
def get_index(s3_client):
    # Request the index file
    try:
        s3_index_obj_request = s3_client.get_object(Bucket=BUCKET_ID, Key=PROJECT_CGP_ROOT_FILE)
    except ClientError as e:
        print e
        raise errors.ClientError("Could not read from the key '{}' in the S3 bucket '{}'.".format(PROJECT_CGP_ROOT_FILE, BUCKET_ID), e)

    # Does the lambda have access to it?
    if s3_index_obj_request['ResponseMetadata']['HTTPStatusCode'] != 200:        
        raise errors.ClientError("The user does not have access to the file index.html file.")

    content = s3_index_obj_request['Body'].read().decode('utf-8')    
    return content

def get_match_group(pattern, content):    
    match = re.search(pattern, content, re.M | re.I)
    return match.group()
    
def get_presigned_url(s3_client, key):
    return s3_client.generate_presigned_url('get_object', Params={'Bucket': BUCKET_ID, 'Key': key}, ExpiresIn=2000, HttpMethod="GET")