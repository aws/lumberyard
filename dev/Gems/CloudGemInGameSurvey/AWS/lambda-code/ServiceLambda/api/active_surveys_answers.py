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
import time

@service.api
def post(request, survey_id, answer_list):
    cognito_identity_id = request.event["cognitoIdentityId"]
    validation_common.validate_cognito_identity_id(cognito_identity_id)
    answers = answer_list.get('answers')
    validation_common.validate_answers(answers)

    survey_common.ensure_survey_active(survey_id)

    question_ids = [answer['question_id'] for answer in answers]
    if len(question_ids) != len(set(question_ids)):
        raise errors.ClientError("Input has duplicate question IDs")

    questions = survey_common.get_questions(survey_id, question_ids)
    if len(questions) != len(question_ids):
        raise errors.ClientError("Some question IDs not found")
    question_map = {}
    for question in questions:
        question_map[question['question_id']] = question

    submission_id = str(uuid.uuid4())

    item = {}
    item['survey_id'] = survey_id
    item['submission_id'] = submission_id
    item['user_id'] = cognito_identity_id
    item['creation_time'] = int(time.time())
    answers_map = {}
    item['answers'] = answers_map
    for answer in answers:
        question = question_map[answer['question_id']]
        validate_answer_by_question_type(question, answer['answer'])
        # for empty text type answer, replace it with a single whitespace
        # as dynamo db doesn't allow empty string..
        if question['type'] == 'text' and len(answer['answer'][0]) == 0:
            answer['answer'][0] = " "
        answers_map[answer['question_id']] = answer['answer']

    survey_utils.get_answer_table().put_item(
        Item=item
    )

    # +1 num_responses
    survey_utils.get_survey_table().update_item(
        Key={'survey_id':survey_id},
        UpdateExpression='ADD num_responses :one',
        ExpressionAttributeValues={':one':1}
    )

    return {
        'submission_id': submission_id
    }

@service.api
def put(request, survey_id, submission_id, answer_list):
    cognito_identity_id = request.event["cognitoIdentityId"]
    validation_common.validate_cognito_identity_id(cognito_identity_id)
    validation_common.validate_survey_id(survey_id)
    validation_common.validate_submission_id(submission_id)
    answers = answer_list.get('answers')
    validation_common.validate_answers(answers)

    survey_common.ensure_survey_active(survey_id)

    submission = survey_utils.get_submission_by_id(survey_id, submission_id, ['user_id'], True)
    if submission.get('user_id') != cognito_identity_id:
        raise errors.ClientError("Cognito identity ID [{}] doesn't own this submission".format(cognito_identity_id))

    question_ids = [answer['question_id'] for answer in answers]
    if len(question_ids) != len(set(question_ids)):
        raise errors.ClientError("Input has duplicate question IDs")

    questions = survey_common.get_questions(survey_id, question_ids)
    if len(questions) != len(question_ids):
        raise errors.ClientError("Some question IDs not found")
    question_map = {}
    for question in questions:
        question_map[question['question_id']] = question

    answers_map = {}
    for answer in answers:
        question = question_map[answer['question_id']]
        validate_answer_by_question_type(question, answer['answer'])
        # for empty text type answer, replace it with a single whitespace
        # as dynamo db doesn't allow empty string..
        if question['type'] == 'text' and len(answer['answer'][0]) == 0:
            answer['answer'][0] = " "
        answers_map[answer['question_id']] = answer['answer']

    survey_utils.get_answer_table().update_item(
        Key={'survey_id':survey_id, 'submission_id': submission_id},
        UpdateExpression='SET answers=:answers',
        ExpressionAttributeValues={':answers': answers_map}
    )

    return {
        'submission_id': submission_id
    }

def validate_answer_by_question_type(question, answer):
    question_type = question['type']
    if question_type == 'text':
        if len(answer[0]) > question['max_chars']:
            raise errors.ClientError("answer to text type question is invalid, number of characters: {}, max allowed: {}".format(len(answer[0]), question['max_chars']))
    elif question_type == 'scale':
        if not validation_utils.is_num_str(answer[0]):
            raise errors.ClientError("answer to scale type question is invalid: {}".format(answer[0]))
        val = int(answer[0])
        if val < question['min'] or val > question['max']:
            raise errors.ClientError("answer to scale type question is invalid: {}, min: {}, max: {}".format(answer[0], question['min'], question['max']))
    elif question_type == 'predefined':
        if question.get('multiple_select'):
            for ans in answer:
                validate_answer_to_predefined_type_question(question, ans)
        else:
            validate_answer_to_predefined_type_question(question, answer[0])

def validate_answer_to_predefined_type_question(question, answer):
    if not validation_utils.is_num_str(answer):
        raise errors.ClientError("answer to predefined type question is invalid: {}".format(answer))
    val = int(answer)
    if val < 0 or val >= len(question['predefines']):
        raise errors.ClientError("answer to predefined type question is invalid: {}, number of options: {}".format(answer, len(question['predefines'])))
