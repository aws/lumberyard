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
import json
import time
import StringIO
import uuid
from botocore.exceptions import ClientError

@service.api
def post(request, survey_id, question):
    validation_common.validate_survey_id(survey_id)
    validation_common.validate_question(question)

    survey_common.ensure_survey_exists(survey_id)

    question_id = str(uuid.uuid4())

    survey_utils.get_survey_table().update_item(
        Key={'survey_id':survey_id},
        UpdateExpression='SET questions = list_append(questions, :question) ADD num_active_questions :one',
        ExpressionAttributeValues={
            ':question': [{'id': question_id, 'active': True}],
            ':one': 1
        }
    )

    item = {}
    item['survey_id'] = survey_id
    item['question_id'] = question_id
    item['metadata'] = survey_common.extract_question_metadata(question)
    item['title'] = question['title']
    item['type'] = question['type']
    survey_utils.get_question_table().put_item(
        Item=item
    )

    return {
        'question_id': question_id
    }

@service.api
def put(request, survey_id, question_id, question):
    validation_utils.validate_param(survey_id, 'survey_id', validation_utils.is_not_blank_str)
    validation_utils.validate_param(question_id, 'question_id', validation_utils.is_not_blank_str)
    validation_common.validate_question(question)

    survey_common.ensure_question_belongs_to_survey(survey_id, question_id)

    item = {}
    item['survey_id'] = survey_id
    item['question_id'] = question_id
    item['metadata'] = survey_common.extract_question_metadata(question)
    item['title'] = question['title']
    item['type'] = question['type']
    survey_utils.get_question_table().put_item(
        Item=item
    )

    return 'success'

@service.api
def delete(request, survey_id, question_id):
    validation_utils.validate_param(survey_id, 'survey_id', validation_utils.is_not_blank_str)
    validation_utils.validate_param(question_id, 'question_id', validation_utils.is_not_blank_str)

    survey_metadata = survey_utils.get_survey_metadata_by_id(survey_id, ['questions'], True)
    for i, question in enumerate(survey_metadata['questions']):
        if question.get('id') == question_id:
            try:
                if question.get('active'):
                    survey_utils.get_survey_table().update_item(
                        Key={'survey_id':survey_id},
                        UpdateExpression='REMOVE questions[{}] ADD num_active_questions :minus_one'.format(i),
                        ConditionExpression='questions[{}].id = :question_id and questions[{}].active = :true'.format(i,i),
                        ExpressionAttributeValues={':question_id':question_id, ':minus_one':-1, ':true': True}
                    )
                else:
                    survey_utils.get_survey_table().update_item(
                        Key={'survey_id':survey_id},
                        UpdateExpression='REMOVE questions[{}]'.format(i),
                        ConditionExpression='questions[{}].id = :question_id and questions[{}].active = :false'.format(i,i),
                        ExpressionAttributeValues={':question_id':question_id, ':false': False}
                    )
            except ClientError as e:
            	if e.response['Error']['Code'] == 'ConditionalCheckFailedException':
            		raise errors.ClientError('Survey has been modified before update')
                else:
                    raise RuntimeError('Failed to update DynamoDB')
            break
    else:
        raise errors.ClientError('No question with id [{}] found for survey [{}]'.format(question_id, survey_id))

    survey_utils.get_question_table().delete_item(
        Key={
            'survey_id': survey_id,
            'question_id': question_id
        }
    )

    return 'success'
