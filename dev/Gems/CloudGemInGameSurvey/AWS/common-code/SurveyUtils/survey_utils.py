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
from boto3.dynamodb.conditions import Key
import CloudCanvas
import errors
import base64
from cgf_utils import custom_resource_utils
from botocore.client import Config

def get_survey_table():
    if not hasattr(get_survey_table, 'survey_table'):
        survey_table_name = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('Surveys'))
        get_survey_table.survey_table = boto3.resource('dynamodb').Table(survey_table_name)
        if get_survey_table.survey_table is None:
            raise RuntimeError('No Survey Table')
    return get_survey_table.survey_table

def get_answer_table():
    if not hasattr(get_answer_table, 'answer_table'):
        answer_table_name = CloudCanvas.get_setting('Answers')
        get_answer_table.answer_table = boto3.resource('dynamodb').Table(answer_table_name)
        if get_answer_table.answer_table is None:
            raise RuntimeError('No Answer Table')
    return get_answer_table.answer_table

def get_answer_aggregation_table():
    if not hasattr(get_answer_aggregation_table, 'answer_aggregation_table'):
        answer_aggregation_table_name = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('AnswerAggregations'))
        get_answer_aggregation_table.answer_aggregation_table = boto3.resource('dynamodb').Table(answer_aggregation_table_name)
        if get_answer_aggregation_table.answer_aggregation_table is None:
            raise RuntimeError('No AnswerAggregation Table')
    return get_answer_aggregation_table.answer_aggregation_table

def get_question_table():
    if not hasattr(get_question_table, 'question_table'):
        question_table_name = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('Questions'))
        get_question_table.question_table = boto3.resource('dynamodb').Table(question_table_name)
        if get_question_table.question_table is None:
            raise RuntimeError('No Question Table')
    return get_question_table.question_table

def get_survey_metadata_by_id(survey_id, attributes=None, raise_exception_if_not_found=False):
    if attributes is not None:
        projection_expression, expression_attribute_names_map = __create_projection_expression_and_attribute_names(attributes)
        get_result = get_survey_table().get_item(
            Key={'survey_id': survey_id},
            ProjectionExpression=projection_expression,
            ExpressionAttributeNames=expression_attribute_names_map
        )
    else:
        get_result = get_survey_table().get_item(
            Key={'survey_id': survey_id}
        )

    survey = get_result.get('Item')
    if raise_exception_if_not_found and survey is None:
        raise errors.ClientError('No survey found for survey_id: [{}]'.format(survey_id))

    return survey

def get_question_by_id(survey_id, question_id, attributes=None, raise_exception_if_not_found=False):
    if attributes is not None:
        projection_expression, expression_attribute_names_map = __create_projection_expression_and_attribute_names(attributes)
        get_result = get_question_table().get_item(
            Key={'survey_id': survey_id, 'question_id': question_id},
            ProjectionExpression=projection_expression,
            ExpressionAttributeNames=expression_attribute_names_map
        )
    else:
        get_result = get_question_table().get_item(
            Key={'survey_id': survey_id}
        )

    question = get_result.get('Item')
    if raise_exception_if_not_found and question is None:
        raise errors.ClientError('No question found for question_id: [{}]'.format(question_id))

    return question

def get_submission_by_id(survey_id, submission_id, attributes=None, raise_exception_if_not_found=False):
    if attributes is not None:
        projection_expression, expression_attribute_names_map = __create_projection_expression_and_attribute_names(attributes)
        get_result = get_answer_table().get_item(
            Key={'survey_id': survey_id, 'submission_id': submission_id},
            ProjectionExpression=projection_expression,
            ExpressionAttributeNames=expression_attribute_names_map
        )
    else:
        get_result = get_answer_table().get_item(
            Key={'survey_id': survey_id, 'submission_id': submission_id}
        )

    submission = get_result.get('Item')
    if raise_exception_if_not_found and submission is None:
        raise errors.ClientError('No submission found for survey_id: [{}], submission_id: [{}]'.format(survey_id, submission_id))

    return submission

def get_answer_submissions_export_s3_bucket():
    if not hasattr(get_answer_submissions_export_s3_bucket, 'answer_submissions_export_s3_bucket'):
        answer_submissions_export_s3_bucket_name = get_answer_submissions_export_s3_bucket_name()
        get_answer_submissions_export_s3_bucket.answer_submissions_export_s3_bucket = boto3.resource('s3', config=Config(signature_version='s3v4')).Bucket(answer_submissions_export_s3_bucket_name)
        if get_answer_submissions_export_s3_bucket.answer_submissions_export_s3_bucket is None:
            raise RuntimeError('No Answer Submission Export S3 Bucket')
    return get_answer_submissions_export_s3_bucket.answer_submissions_export_s3_bucket

def get_answer_submissions_export_s3_bucket_name():
    return CloudCanvas.get_setting('AnswerSubmissionsExportS3Bucket')

def __create_projection_expression_and_attribute_names(attributes):
    attribute_aliases = []
    expression_attribute_names_map = {}
    for i, attribute in enumerate(attributes):
        alias = '#a{}'.format(i)
        attribute_aliases.append(alias)
        expression_attribute_names_map[alias] = attribute
    projection_expression = ','.join(attribute_aliases)
    return projection_expression, expression_attribute_names_map
