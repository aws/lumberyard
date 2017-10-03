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
import CloudCanvas
import survey_utils
import survey_common
import validation_utils
import validation_common
import uuid
import errors
from boto3.dynamodb.conditions import Key

MAX_LIMIT = 1000

@service.api
def get(request, survey_id, limit=None, pagination_token=None, sort=None):
    limit = validation_common.validate_limit(limit, MAX_LIMIT)
    sort = validation_common.validate_sort_order(sort)

    return survey_common.get_answer_submissions(survey_id, limit, pagination_token, sort)


@service.api
def delete(request, survey_id, submission_id):
    survey_utils.get_answer_table().delete_item(
        Key={
            'survey_id': survey_id,
            'submission_id': submission_id
        }
    )

    return {
        'status': 'success'
    }
