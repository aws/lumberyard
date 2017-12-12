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
import validation_utils
import validation_common
import errors
from botocore.exceptions import ClientError
from boto3.dynamodb.conditions import Key
import json
import time
import base64
from decimal import Decimal

def get_survey(survey_id, question_index, question_count, ensure_active, is_admin):
    question_index, end_index = validation_common.validate_question_range_param(question_index, question_count)

    survey_metadata = survey_utils.get_survey_metadata_by_id(
        survey_id, ['questions', 'survey_name', 'activation_start_time', 'activation_end_time', 'published'], True)

    if ensure_active:
        ensure_survey_active_from_metadata(survey_metadata, survey_id)

    question_ids = map(lambda x: x['id'], survey_metadata['questions'])
    question_active_flags = map(lambda x: x['active'], survey_metadata['questions'])
    if is_admin:
        question_ids = question_ids[question_index:end_index]
    else:
        question_ids = [x for i, x in enumerate(question_ids) if question_active_flags[i]][question_index:end_index]

    questions = get_questions(survey_id, question_ids)

    if is_admin:
        for i, enabled in enumerate(question_active_flags[question_index:end_index]):
            questions[i]['enabled'] = enabled

    return {
        'survey_id': survey_id,
        'survey_name': survey_metadata.get('survey_name'),
        'questions': questions
    }

def get_survey_question_ids(survey_id):
    params = {}
    params['KeyConditionExpression'] = 'survey_id = :survey_id'
    params['ExpressionAttributeValues'] = {':survey_id': survey_id}
    params['ProjectionExpression'] ='question_id'
    question_ids = []
    while True:
        query_result = survey_utils.get_question_table().query(**params)
        for question in query_result['Items']:
            question_ids.append(question['question_id'])
        last_evaluated_key = query_result.get('LastEvaluatedKey')
        if last_evaluated_key is None:
            break
        params['ExclusiveStartKey'] = last_evaluated_key
    return question_ids

def get_questions(survey_id, question_ids):
    if len(question_ids) == 0:
        return []

    question_map = {}
    params = {}
    params['KeyConditionExpression'] = 'survey_id = :survey_id'
    params['ExpressionAttributeValues'] = {':survey_id': survey_id}
    params['ProjectionExpression'] ='question_id, title, #type, metadata'
    params['ExpressionAttributeNames'] = {'#type':'type'}
    while True:
        query_result = survey_utils.get_question_table().query(**params)
        for question in query_result['Items']:
            question_map[question['question_id']] = question

        last_evaluated_key = query_result.get('LastEvaluatedKey')
        if last_evaluated_key is None:
            break
        params['ExclusiveStartKey'] = last_evaluated_key

    # only return questions that actually exists in Question table
    questions = []
    for question_id in question_ids:
        question = question_map.get(question_id)
        if question:
            # flatten metadata
            metadata = question.get('metadata')
            if metadata is not None:
                question.update(metadata)
                del question['metadata']
            questions.append(question)
    return questions

def get_survey_metadata_by_id(survey_id, ensure_active):
    survey_metadata = survey_utils.get_survey_metadata_by_id(
        survey_id, ['survey_id', 'survey_name', 'creation_time', 'num_active_questions', 'activation_start_time', 'activation_end_time', 'published'], True)

    if ensure_active:
        ensure_survey_active_from_metadata(survey_metadata)

    __remove_or_populate_published_attribute(ensure_active, survey_metadata)

    return {
        'metadata_list':[survey_metadata]
    }

def get_survey_metadata_list(limit, max_limit, pagination_token, sort, ensure_active):
    limit = validation_common.validate_limit(limit, max_limit)
    sort = validation_common.validate_sort_order(sort)

    params = {}
    params['KeyConditionExpression'] = 'creation_time_dummy_hash = :true'
    params['ExpressionAttributeValues'] = {':true': 1}
    params['ProjectionExpression'] = 'survey_id, survey_name, creation_time, num_active_questions, activation_start_time, activation_end_time, published, num_responses'
    params['Limit'] = limit
    params['IndexName'] = 'CreationTimeIndex'

    if sort == 'desc':
        params['ScanIndexForward'] = False

    if ensure_active:
        params['FilterExpression'] = ':current_time >= activation_start_time and (attribute_not_exists(activation_end_time) or :current_time <= activation_end_time) and published = :true'
        params['ExpressionAttributeValues'][':current_time'] = int(time.time())

    if validation_utils.is_not_empty(pagination_token):
        params['ExclusiveStartKey'] = decode_pagination_token(pagination_token)

    scan_result = survey_utils.get_survey_table().query(**params)

    out = {'metadata_list': map(lambda x: __remove_or_populate_published_attribute(ensure_active, x), scan_result['Items'])}
    last_evaluated_key = scan_result.get('LastEvaluatedKey')
    if last_evaluated_key is not None:
        out['pagination_token'] = encode_pagination_token(last_evaluated_key)

    return out

def __remove_or_populate_published_attribute(ensure_active, survey_metadata):
    if ensure_active:
        try:
            del survey_metadata['published']
        except:
            pass
    else:
        survey_metadata['published'] = True if survey_metadata.get('published') == 1 else False
    return survey_metadata

def encode_pagination_token(last_evaluated_key):
    for k,v in last_evaluated_key.items():
        if isinstance(v, Decimal):
            if v % 1 > 0:
                last_evaluated_key[k] = float(v)
            else:
                last_evaluated_key[k] = int(v)
    return base64.b64encode(json.dumps(last_evaluated_key))

def decode_pagination_token(pagination_token):
    try:
        last_evaluated_key = json.loads(base64.b64decode(pagination_token))
        for k,v in last_evaluated_key.items():
            if isinstance(v, int) or isinstance(v, float):
                last_evaluated_key[k] = Decimal(v)
        return last_evaluated_key
    except:
        raise errors.ClientError('pagination_token is invalid: [{}]'.format(pagination_token))

def ensure_survey_exists(survey_id):
    survey_utils.get_survey_metadata_by_id(survey_id, ['survey_id'], True)

def ensure_survey_active(survey_id):
    survey_metadata = survey_utils.get_survey_metadata_by_id(
        survey_id, ['activation_start_time', 'activation_end_time', 'published'], True)
    ensure_survey_active_from_metadata(survey_metadata, survey_id)

def ensure_question_belongs_to_survey(survey_id, question_id):
    question = survey_utils.get_question_by_id(survey_id, question_id, ['survey_id'], True)
    if survey_id != question['survey_id']:
        raise errors.ClientError("Question {} doesn't belong to survey {}".format(question_id, survey_id))

def ensure_question_exists(survey_id, question_id):
    survey_utils.get_question_by_id(survey_id, question_id, ['question_id'], True)

def ensure_survey_active_from_metadata(survey_metadata, survey_id=None):
    cur_time = int(time.time())
    if survey_metadata.get('published') != 1 or cur_time < survey_metadata['activation_start_time'] or (survey_metadata.get('activation_end_time') is not None and cur_time > survey_metadata.get('activation_end_time')):
        raise errors.ForbiddenRequestError('Survey is not active for survey_id: [{}]'.format(survey_id if survey_id else survey_metadata['survey_id']))

def get_answer_submission_ids(survey_id, limit, pagination_token, sort):
    params = {}
    params['KeyConditionExpression'] = Key('survey_id').eq(survey_id)
    params['IndexName'] = 'SubmissionTimeIndex'
    params['ProjectionExpression'] = 'submission_id'
    if limit is not None:
        params['Limit'] = limit
    if pagination_token is not None:
        params['ExclusiveStartKey'] = decode_pagination_token(pagination_token)
    if sort.lower() == 'desc':
        params['ScanIndexForward'] = False

    query_result = survey_utils.get_answer_table().query(**params)

    submission_ids = []
    for submission in query_result['Items']:
        submission_ids.append(submission['submission_id'])

    out = {'submission_ids': submission_ids}
    last_evaluated_key = query_result.get('LastEvaluatedKey')
    if last_evaluated_key is not None:
        out['pagination_token'] = encode_pagination_token(last_evaluated_key)

    return out

def get_answer_submission(survey_id, submission_id):
    get_result = survey_utils.get_answer_table().get_item(Key={
        'survey_id': survey_id,
        'submission_id': submission_id
    })
    submission = get_result.get('Item')
    if submission is None:
        return None

    del submission['survey_id']
    del submission['user_id']
    __convert_answers_map_to_list(submission)

    return submission

def get_answer_submissions(survey_id, limit, pagination_token, sort):
    params = {}
    params['KeyConditionExpression'] = Key('survey_id').eq(survey_id)
    params['IndexName'] = 'SubmissionTimeIndex'
    if limit is not None:
        params['Limit'] = limit
    if pagination_token is not None:
        params['ExclusiveStartKey'] = decode_pagination_token(pagination_token)
    if sort.lower() == 'desc':
        params['ScanIndexForward'] = False

    query_result = survey_utils.get_answer_table().query(**params)

    submissions = []
    for item in query_result['Items']:
        del item['survey_id']
        __convert_answers_map_to_list(item)
        submissions.append(item)

    out = {'submissions': submissions}
    last_evaluated_key = query_result.get('LastEvaluatedKey')
    if last_evaluated_key is not None:
        out['pagination_token'] = encode_pagination_token(last_evaluated_key)

    return out

def __convert_answers_map_to_list(submission):
    answers_map = submission['answers']
    answer_list = []
    for question_id, answer in answers_map.items():
        answer_list.append({
            'question_id': question_id,
            'answer': answer
        })
    submission['answers'] = answer_list

def extract_question_metadata(question):
    metadata_map = {}
    for key, val in question.items():
        if key in extract_question_metadata.metadata_fields:
            metadata_map[key] = val
    return metadata_map

extract_question_metadata.metadata_fields = set(['max_chars', 'max', 'min', 'multiple_select', 'predefines', 'min_label', 'max_label'])
