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
import validation_utils
import validation_common
import uuid
import errors
from boto3.dynamodb.conditions import Key
import base64

@service.api
def get(request, survey_id):
    validation_common.validate_survey_id(survey_id)

    query_result = survey_utils.get_answer_aggregation_table().query(
        KeyConditionExpression=Key('survey_id').eq(survey_id)
    )

    survey_answer_aggregations = []
    for each_survey_answer_aggregation in query_result['Items']:
        for key, val in each_survey_answer_aggregation.items():
            if key == 'survey_id':
                continue
            question_id = key
            each_question_answer_aggregation = val

            answer_aggregations = []
            question_answer_aggregations = {
                'question_id': question_id,
                'answer_aggregations': answer_aggregations
            }

            for answer, count in each_question_answer_aggregation.items():
                answer_aggregations.append({
                    'answer': answer,
                    'count': count
                })

            survey_answer_aggregations.append(question_answer_aggregations)

    return {
        'question_answer_aggregations': survey_answer_aggregations
    }
