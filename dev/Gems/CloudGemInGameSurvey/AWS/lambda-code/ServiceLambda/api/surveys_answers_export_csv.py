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

import service
import survey_common
import survey_utils
import validation_utils
import validation_common
import errors
import json
import time
import boto3
import CloudCanvas
from botocore.client import Config

@service.api
def get(request, survey_id, request_id):
    bucket_name = survey_utils.get_answer_submissions_export_s3_bucket_name()
    status_file_key = get_status_file_key(request_id)

    s3 = boto3.client('s3', config=Config(signature_version='s3v4'))
    response = s3.get_object(Bucket=bucket_name, Key=status_file_key)
    status = json.loads(response["Body"].read())

    out = {
        'num_submissions_exported': status.get('num_submissions_exported')
    }

    if status.get('finished'):
        out['s3_presigned_url'] = status.get('presigned_url')

    return out

@service.api
def post(request, survey_id):
    validation_common.validate_survey_id(survey_id)

    timestamp = str(int(time.time()*1000))
    request_id = get_request_id(survey_id, timestamp)
    status_file_key = get_status_file_key(request_id)

    # create status file
    create_status_file(status_file_key)

    # initiate multipart upload
    export_file_key = get_export_file_key(request_id)
    s3 = boto3.client('s3', config=Config(signature_version='s3v4'))
    bucket_name = survey_utils.get_answer_submissions_export_s3_bucket_name()
    upload = s3.create_multipart_upload(Bucket=bucket_name, Key=export_file_key)

    upload_info = {
        'survey_id': survey_id,
        'bucket_name': bucket_name,
        'num_submissions_exported': 0,
        'part_num': 1,
        'upload_id': upload['UploadId'],
        'export_file_key': export_file_key,
        'status_file_key': status_file_key,
        'parts_info':{
            'Parts':[]
        },
        'pagination_token': None
    }

    boto3.client('lambda').invoke_async(
        FunctionName=CloudCanvas.get_setting('ExportAnswerSubmissionsCSVLambda'),
        InvokeArgs=json.dumps(upload_info)
    )

    return {
        "request_id": request_id
    }

def get_request_id(suvey_id, timestamp):
    return '{}_{}'.format(suvey_id, timestamp)

def get_status_file_key(request_id):
    return '{}/{}'.format(request_id, 'status.json')

def get_export_file_key(request_id):
    return '{}/{}'.format(request_id, 'export.csv')

def create_status_file(status_file_key):
    bucket = survey_utils.get_answer_submissions_export_s3_bucket()
    status = {
        "num_submissions_exported": 0,
        "finished": False
    }

    bucket.put_object(Key=status_file_key, Body=json.dumps(status))
