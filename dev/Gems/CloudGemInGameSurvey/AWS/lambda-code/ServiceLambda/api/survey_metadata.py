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
import errors
import survey_utils
import survey_common
import validation_utils
import validation_common

MAX_LIMIT = 1000

@service.api
def get(request, survey_name=None, limit=None, pagination_token=None, sort=None):
    if validation_utils.is_empty(survey_name):
        return survey_common.get_survey_metadata_list(limit, MAX_LIMIT, pagination_token, sort, False)
    else:
        return search_survey_metadata_by_survey_name(survey_name, limit, pagination_token, sort)

def search_survey_metadata_by_survey_name(survey_name, limit, pagination_token, sort):
    limit = validation_common.validate_limit(limit, MAX_LIMIT)
    sort = validation_common.validate_sort_order(sort)

    params = {}
    params['KeyConditionExpression'] = 'creation_time_dummy_hash = :true'
    params['ProjectionExpression'] = 'survey_id, survey_name, creation_time, num_active_questions, activation_start_time, activation_end_time, published, num_responses'
    params['Limit'] = limit
    params['FilterExpression'] = 'contains(survey_name, :survey_name)'
    params['ExpressionAttributeValues'] = {':survey_name':survey_name, ':true':1}
    params['IndexName'] = 'CreationTimeIndex'

    if sort == 'desc':
        params['ScanIndexForward'] = False

    if pagination_token is not None:
        params['ExclusiveStartKey'] = survey_common.decode_pagination_token(pagination_token)

    scan_result = survey_utils.get_survey_table().query(**params)

    out = {'metadata_list':scan_result['Items']}
    last_evaluated_key = scan_result.get('LastEvaluatedKey')
    if last_evaluated_key is not None:
        out['pagination_token'] = survey_common.encode_pagination_token(last_evaluated_key)

    return out
