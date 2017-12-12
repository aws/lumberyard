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
import errors
import json
import time
import uuid
import CloudCanvas
import boto3

@service.api
def get(request, survey_id, question_index=None, question_count=None):
    return survey_common.get_survey(survey_id, question_index, question_count, False, True)

@service.api
def post(request, create_survey_input):
    survey_name = create_survey_input.get('survey_name')
    validation_utils.validate_param(survey_name, 'survey_name', validation_utils.is_not_blank_str)

    survey_id_to_clone = create_survey_input.get('survey_id_to_clone')
    if survey_id_to_clone:
        return clone_survey(survey_name, survey_id_to_clone)
    else:
        return create_survey(survey_name)

@service.api
def delete(request, survey_id):
    delete_survey(survey_id)
    delete_questions(survey_id)
    delete_answer_aggregations(survey_id)
    delete_answer_submissions_async(survey_id)

    return {
        'status': 'success'
    }

def delete_survey(survey_id):
    survey_utils.get_survey_table().delete_item(
        Key={
            'survey_id': survey_id
        }
    )

def delete_questions(survey_id):
    question_table = survey_utils.get_question_table()
    question_ids = survey_common.get_survey_question_ids(survey_id)
    for question_id in question_ids:
        question_table.delete_item(
            Key={
                'survey_id': survey_id,
                'question_id': question_id
            }
        )

def delete_answer_aggregations(survey_id):
    survey_utils.get_answer_aggregation_table().delete_item(
        Key={
            'survey_id': survey_id
        }
    )

def delete_answer_submissions_async(survey_id):
    delete_submissions_info = {
        'survey_id': survey_id,
        'pagination_token': None,
        'submission_id_list': [],
    }

    boto3.client('lambda').invoke_async(
        FunctionName=CloudCanvas.get_setting('DeleteAnswerSubmissionsLambda'),
        InvokeArgs=json.dumps(delete_submissions_info)
    )

def clone_survey(survey_name, survey_id_to_clone):
    survey = survey_common.get_survey(survey_id_to_clone, None, None, False, True)
    survey_id = str(uuid.uuid4())
    questions = []
    for question in survey['questions']:
        question_id = str(uuid.uuid4())
        questions.append({
            'id': question_id,
            'active': True
        })
        item = {}
        item['survey_id'] = survey_id
        item['question_id'] = question_id
        item['metadata'] = survey_common.extract_question_metadata(question)
        item['title'] = question['title']
        item['type'] = question['type']
        survey_utils.get_question_table().put_item(
            Item=item
        )

    creation_time = int(time.time())
    survey_utils.get_survey_table().put_item(
        Item={
            'survey_id': survey_id,
            'survey_name': survey_name,
            'creation_time_dummy_hash': 1,
            'creation_time': creation_time,
            'activation_start_time': 0,
            'num_active_questions': len(questions),
            'questions': questions,
            'num_responses': 0
        }
    )

    return {
        'survey_id': survey_id,
        'creation_time': creation_time
    }

def create_survey(survey_name):
    survey_id = str(uuid.uuid4())
    creation_time = int(time.time())
    survey_utils.get_survey_table().put_item(
        Item={
            'survey_id': survey_id,
            'survey_name': survey_name,
            'creation_time_dummy_hash': 1,
            'creation_time': creation_time,
            'activation_start_time': 0,
            'num_active_questions': 0,
            'questions': [],
            'num_responses': 0
        }
    )

    return {
        'survey_id': survey_id,
        'creation_time': creation_time
    }
