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
import survey_utils
import survey_common
import CloudCanvas
import json

def main(delete_submissions_info, context):
    survey_id = delete_submissions_info['survey_id']
    submission_id_list = delete_submissions_info['submission_id_list']

    idx = 0
    no_more_submission = False
    while True:
        if idx >= len(submission_id_list):
            if no_more_submission:
                break
            # load at most next 1000 submission ids to avoid exceeding the max payload limit to lambda 128k
            result = survey_common.get_answer_submission_ids(survey_id, 1000, delete_submissions_info['pagination_token'], 'desc')
            if len(result['submission_ids']) == 0:
                break
            submission_id_list.extend(result['submission_ids'])

            delete_submissions_info['pagination_token'] = result.get('pagination_token')
            if delete_submissions_info['pagination_token'] is None or len(delete_submissions_info['pagination_token']) == 0:
                no_more_submission = True

        submission_id = submission_id_list[idx]
        idx += 1
        survey_utils.get_answer_table().delete_item(
            Key={
                'survey_id': survey_id,
                'submission_id': submission_id
            }
        )

        if should_trigger_next_lambda(context):
            delete_submissions_info['submission_id_list'] = submission_id_list[idx:]
            trigger_next_lambda(delete_submissions_info, context)
            break

def should_trigger_next_lambda(context):
    # trigger another lambda if remaining time is less than 30 seconds
    if context.get_remaining_time_in_millis() < 30000:
        return True
    else:
        return False

def trigger_next_lambda(delete_submissions_info, context):
    boto3.client('lambda').invoke_async(
        FunctionName=context.invoked_function_arn,
        InvokeArgs=json.dumps(delete_submissions_info)
    )
