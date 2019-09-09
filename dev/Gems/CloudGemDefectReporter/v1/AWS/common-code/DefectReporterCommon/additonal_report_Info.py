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
import botocore
import CloudCanvas

from errors import ClientError
from cgf_utils import custom_resource_utils

def query_report_headers():
    response = __get_table().query(
        ConsistentRead=False,
        IndexName='SectionIndex',
        KeyConditionExpression=Key('section').eq('header')
    )

    report_headers = response.get('Items', [])
    response = [
        {
            "universal_unique_identifier": report_header['universal_unique_identifier'],
            "bookmark": report_header.get('value', {}).get('bookmark', 0),
            "report_status": report_header.get('value', {}).get('report_status', 'unread'),
            'jira_status': report_header.get('value', {}).get('jira_status', 'pending')
        } for report_header in report_headers]

    return response

def update_report_header(report):
    if 'universal_unique_identifier' not in report:
        raise ClientError("Could not find the uuid of this report")

    key = { 'universal_unique_identifier': report.get('universal_unique_identifier', ''), 'section': 'header' }
    existing_item = __get_table().get_item(Key=key).get('Item', {})
    header_value = existing_item.get('value', {})

    # Set the header values according to the report properties
    # When the report doesn't have the required headers, keep the existing value in the table
    header_value['jira_status'] = report['jira_status'] if report.get('jira_status') != None else header_value.get('jira_status', 'pending')
    header_value['bookmark'] = report['bookmark'] if report.get('bookmark') != None else header_value.get('bookmark', 0)
    header_value['report_status'] = report['report_status'] if report.get('report_status') != None else header_value.get('report_status', 'unread')

    report_header = {
        'universal_unique_identifier': report.get('universal_unique_identifier', ''),
        'section': 'header',
        'value': header_value
        }

    __get_table().put_item(Item=report_header)

    return 'SUCCESS'

def get_report_comments(universal_unique_identifier):
    key = { 'universal_unique_identifier': universal_unique_identifier, 'section': 'comments' }
    report_comments = __get_table().get_item(Key=key).get('Item', {})
    response = {
        'universal_unique_identifier': report_comments.get('universal_unique_identifier', ''),
        'comments': report_comments.get('value', {}).get('comments', [])}
    return response

def update_report_comment(report):
    if not report['universal_unique_identifier']:
        raise ClientError("Could not find the uuid of this report")

    key = { 'universal_unique_identifier': report['universal_unique_identifier'], 'section': 'comments' }
    report_comments = {
        'universal_unique_identifier': report['universal_unique_identifier'],
        'section': 'comments',
        'value': {
            'comments': report.get('comments', []),
            }
        }
    __get_table().put_item(Item=report_comments)

    return 'SUCCESS'

def __get_table():
    if not hasattr(__get_table,'additional_report_info'):
        __get_table.additional_report_info = boto3.resource('dynamodb').Table(custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('AdditionalReportInfo')))
    return __get_table.additional_report_info