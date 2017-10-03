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

import json
import boto3
import survey_utils
import survey_common
import sys
import StringIO
import CloudCanvas
import zlib

def handle(upload_info, context):
    part_data = StringIO.StringIO()
    is_end = False
    while True:
        result = survey_common.get_answer_submissions(upload_info['survey_id'], None, upload_info['pagination_token'], 'desc')
        upload_info['pagination_token'] = result.get('pagination_token')
        submissions = result.get('submissions')
        if submissions is not None:
            upload_info['num_submissions_exported'] += len(submissions)
            for submission in submissions:
                answers = submission.get('answers')
                if answers is not None:
                    for answer in answers:
                        part_data.write(answer['question_id'])
                        part_data.write(',')
                        part_data.write('"{}"'.format(",".join(answer['answer'])))
                        part_data.write('\n')
                    part_data.write('\n')
        if not upload_info['pagination_token']:
            is_end = True
            break
        if is_collect_enough(part_data):
            break;

    s3 = boto3.client('s3')
    upload_part(s3, part_data.getvalue(), upload_info)

    if is_end:
        update_status_file(upload_info, True, s3)
        complete_multipart_upload(s3, upload_info)
    else:
        update_status_file(upload_info, False)
        trigger_next_upload(upload_info, context)

def upload_part(s3, part_data, upload_info):
    upload = s3.upload_part(Bucket=upload_info['bucket_name'], Key=upload_info['export_file_key'],
    PartNumber=upload_info['part_num'], UploadId=upload_info['upload_id'], Body=part_data)
    upload_info['parts_info']['Parts'].append({
        'PartNumber': upload_info['part_num'],
        'ETag': upload['ETag']
    });

def complete_multipart_upload(s3, upload_info):
    s3.complete_multipart_upload(Bucket=upload_info['bucket_name'], Key=upload_info['export_file_key'],
    UploadId=upload_info['upload_id'], MultipartUpload=upload_info['parts_info'])

def trigger_next_upload(upload_info, context):
    upload_info['part_num'] += 1
    boto3.client('lambda').invoke_async(
        FunctionName=context.invoked_function_arn,
        InvokeArgs=json.dumps(upload_info)
    )

def update_status_file(upload_info, finished, s3=None):
    bucket = survey_utils.get_answer_submissions_export_s3_bucket()
    status = {
        "num_submissions_exported": upload_info['num_submissions_exported'],
        "finished": finished
    }

    if finished:
        presigned_url = s3.generate_presigned_url('get_object', Params = { "Bucket" : upload_info['bucket_name'], "Key" : upload_info['export_file_key'] })
        status['presigned_url'] = presigned_url

    bucket.put_object(Key=upload_info['status_file_key'], Body=json.dumps(status))

def is_collect_enough(part_data):
    # 5MB is the minimum part size (except for last part) required by multipart upload
    MIN_PART_SIZE = 5*1024**2
    if part_data.len >= MIN_PART_SIZE:
        return True
    else:
        return False
