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
def put(request, survey_id, question_id, question_status):
    validation_common.validate_survey_id(survey_id)
    validation_common.validate_question_id(question_id)
    enabled = question_status.get('enabled')
    validation_utils.validate_param(enabled, 'enabled', validation_utils.is_not_none)

    if enabled:
        return enable_survey_question(survey_id, question_id)
    else:
        return disable_survey_question(survey_id, question_id)

def enable_survey_question(survey_id, question_id):
    survey_metadata = survey_utils.get_survey_metadata_by_id(survey_id, ['questions'], True)
    for i, question in enumerate(survey_metadata['questions']):
        if question['id'] == question_id:
            if question['active'] == False:
                try:
                    survey_utils.get_survey_table().update_item(
                        Key={'survey_id':survey_id},
                        UpdateExpression='SET questions[{}].active = :true ADD num_active_questions :one'.format(i),
                        ConditionExpression='questions[{}].id = :question_id and questions[{}].active = :false'.format(i,i),
                        ExpressionAttributeValues={':true': True, ':false': False, ':question_id':question_id, ':one':1}
                    )
                except ClientError as e:
                    if e.response['Error']['Code'] == 'ConditionalCheckFailedException':
                        raise errors.ClientError('Survey has been modified before update')
                    else:
                        raise RuntimeError('Failed to update DynamoDB')
            break
    else:
        raise errors.ClientError('No question with id [{}] found for survey [{}]'.format(question_id, survey_id))

    return 'success'

def disable_survey_question(survey_id, question_id):
    survey_metadata = survey_utils.get_survey_metadata_by_id(survey_id, ['questions'], True)
    for i, question in enumerate(survey_metadata['questions']):
        if question['id'] == question_id:
            if question['active'] == True:
                try:
                    survey_utils.get_survey_table().update_item(
                        Key={'survey_id':survey_id},
                        UpdateExpression='SET questions[{}].active = :false ADD num_active_questions :minus_one'.format(i),
                        ConditionExpression='questions[{}].id = :question_id and questions[{}].active = :true'.format(i,i),
                        ExpressionAttributeValues={':true': True, ':false': False, ':question_id':question_id, ':minus_one':-1}
                    )
                except ClientError as e:
                    if e.response['Error']['Code'] == 'ConditionalCheckFailedException':
                        raise errors.ClientError('Survey has been modified before update')
                    else:
                        raise RuntimeError('Failed to update DynamoDB')
            break
    else:
        raise errors.ClientError('No question with id [{}] found for survey [{}]'.format(question_id, survey_id))

    return 'success'
