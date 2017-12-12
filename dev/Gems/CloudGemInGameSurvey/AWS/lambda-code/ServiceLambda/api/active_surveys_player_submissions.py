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
from boto3.dynamodb.conditions import Key

@service.api
def get(request, survey_id):
    cognito_identity_id = request.event["cognitoIdentityId"]

    params = {}
    params['KeyConditionExpression'] = Key('user_id').eq(cognito_identity_id) & Key('survey_id').eq(survey_id)
    params['IndexName'] = 'UserAnswersIndex'
    params['ProjectionExpression'] = 'submission_id'

    query_result = survey_utils.get_answer_table().query(**params)

    submission_ids = []
    for submission in query_result['Items']:
        submission_ids.append(submission['submission_id'])

    return {
        'submission_id_list': submission_ids
    }
