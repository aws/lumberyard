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

@service.api
def put(request, survey_id, question_order):
    validation_common.validate_survey_id(survey_id)

    question_id_list = question_order.get('question_id_list')
    validation_utils.validate_param(question_id_list, 'question_id_list', validation_utils.is_not_none)

    survey_metadata = survey_utils.get_survey_metadata_by_id(survey_id, ['questions'], True)
    question_ids = map(lambda x: x['id'], survey_metadata['questions'])
    if set(question_ids) != set(question_id_list):
        raise errors.ClientError("Question IDs from input doesn't match existing question IDs")

    question_map = {}
    for question in survey_metadata['questions']:
        question_map[question['id']] = question

    new_question_list = [question_map[question_id] for question_id in question_id_list]

    survey_utils.get_survey_table().update_item(
        Key={'survey_id':survey_id},
        UpdateExpression='SET questions = :new_question_list',
        ExpressionAttributeValues={':new_question_list':new_question_list}
    )

    return "success"
