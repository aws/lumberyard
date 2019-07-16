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

from __future__ import print_function

import boto3
import datetime
import json
import dateutil

from botocore.exceptions import ClientError

import CloudCanvas

import errors
import service

from cgf_utils import aws_utils
from cgf_utils import custom_resource_utils

import fleet

# import errors
#
# raise errors.ClientError(message) - results in HTTP 400 response with message
# raise errors.ForbiddenRequestError(message) - results in 403 response with message
# raise errors.NotFoundError(message) - results in HTTP 404 response with message
#
# Any other exception results in HTTP 500 with a generic internal service error message.

workflow = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('Workflow'))
workflow_domain_name = workflow + '-domain'
workflow_type_name = workflow + '-workflow-type'

swf_client = boto3.client('swf', region_name=aws_utils.current_region)
dynamo_client = boto3.client('dynamodb')

kvs_table = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('KVS'))
active_workflow_key = 'active_workflow'


def _serialize_json_datetime(obj):
    if isinstance(obj, (datetime.datetime, datetime.date)):
        return obj.isoformat()

    raise TypeError("Type %s is not serializable" % type(obj))


def _init_active_workflow():
    active_workflow = None

    response = dynamo_client.query(
        TableName=kvs_table,
        ConsistentRead=True,
        Select="ALL_ATTRIBUTES",
        KeyConditions={
            'key': {
                'AttributeValueList': [
                    {'S': active_workflow_key}
                ],
                'ComparisonOperator': "EQ"
            }
        }
    )

    if len(response['Items']):
        active_workflow = json.loads(response['Items'][0]['value']['S'])

    return active_workflow


def get_workflow(workflow_id):
    if not workflow_id or len(workflow_id) == 0:
        workflow_id = _init_active_workflow()
    else:
        workflow_id = json.loads(workflow_id)

    result = {}

    if workflow_id:
        result = swf_client.describe_workflow_execution(
            domain=workflow_domain_name,
            execution=workflow_id
        )

    return json.dumps(result, default=_serialize_json_datetime)


class utcinfo(datetime.tzinfo):
    def utcoffset(self, dt):
        return datetime.timedelta(0)

    def tzname(self, dt):
        return 'UTC'

    def dst(self, dt):
        return datetime.timedelta(0)


def get_minutes_since_activity(config_timestamp):
    workflow_status = get_workflow(None)
    workflow_data = json.loads(workflow_status)
    last_activity_timer = workflow_data.get('latestActivityTaskTimestamp')
    datetime_struct = None

    if not last_activity_timer:
        print('No activity timer found')
        start_timer = workflow_data.get('executionInfo',{}).get('startTimestamp')
        if start_timer:
            print('Using start timer')
            datetime_struct = dateutil.parser.parse(start_timer)
    else:
        datetime_struct = dateutil.parser.parse(last_activity_timer)
    if config_timestamp:
        config_struct = dateutil.parser.parse(config_timestamp)
        if not datetime_struct or config_struct > datetime_struct:
            print('Using more recent configuration time of {}'.format(config_timestamp))
            datetime_struct = config_struct

    if not datetime_struct:
        print('No timestamps found')
        return

    current_time = datetime.datetime.now(utcinfo())

    minute_diff = (current_time - datetime_struct).seconds / 60 + (current_time - datetime_struct).days * 1440
    return minute_diff