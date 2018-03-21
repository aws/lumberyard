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
# $Revision$


import json
import boto3
import time
import random

from cgf_utils import custom_resource_response
from cgf_utils import properties
from resource_manager_common import stack_info
from cgf_utils import role_utils
from cgf_utils import patch
from cgf_utils import aws_utils
from cgf_utils import json_utils
from botocore.exceptions import ClientError


class _TableResponse(object):
    def __init__(self, response):
        description = response.get('TableDescription', {})
        self.table_name = description.get('TableName', "")
        self.output = {
            'Arn': description.get('TableArn', ""),
            'StreamArn': description.get('LatestStreamArn', "")
        }


MAX_ATTEMPTS = 10
BACKOFF_MAX = 40.0
BACKOFF_BASE =  0.25

def handler(event, context):
    dynamodb = aws_utils.ClientWrapper(boto3.client('dynamodb'))
    wait_for_account_tables()

    throughput = get_throughput_from_dict(event['ResourceProperties']['ProvisionedThroughput'])

    request_type = event['RequestType']

    table_resource_name = ""
    table_name = get_table_name(event)
    stream_specification = event["ResourceProperties"].get("StreamSpecification", {})

    if request_type == 'Create':
        try:
            if table_name in gather_tables(dynamodb):
                raise RuntimeError("Trying to create a Custom::DynamoDB::Table custom resource, but DynamoDB table already exists!")
            try:
                response = create_table(table_name, event)
            except Exception as e:
                if isinstance(e, ClientError) and e.response['Error']['Code'] in ['LimitExceededException']:
                    wait_for_account_tables()
                    response = create_table(table_name, event)
                else:
                    response = {}
            table_response = _TableResponse(response)
        except RuntimeError as e:
            return custom_resource_response.failure_response(e.message)

    elif request_type == 'Update':
        try:
            if not table_name in gather_tables(dynamodb):
                try:
                    response = create_table(table_name, event)
                except Exception as e:
                    if isinstance(e, ClientError) and e.response['Error']['Code'] in ['LimitExceededException']:
                        wait_for_account_tables()
                        response = create_table(table_name, event)
                    else:
                        raise e

                table_response = _TableResponse(response)

            else:
                try:
                    response = update_table(table_name, event)
                except Exception as e:
                    if isinstance(e, ClientError) and e.response['Error']['Code'] in ['LimitExceededException']:
                        wait_for_account_tables()
                        response = update_table(table_name, event)
                    else:
                        raise e
                table_response = _TableResponse(response)
        except RuntimeError as e:
            return custom_resource_response.failure_response(e.message)

    elif request_type == 'Delete':
        try:
            if table_name in gather_tables(dynamodb):
                try:
                    response = dynamodb.delete_table(TableName = table_name)
                except Exception as e:
                    if isinstance(e, ClientError) and e.response['Error']['Code'] in ['LimitExceededException']:
                        wait_for_account_tables()
                        response = dynamodb.delete_table(TableName = table_name)
                    else:
                        raise e
                table_response = _TableResponse(response)
            else:
                print "Custom::DynamoDB::Table is trying to delete a DynamoDB table that does not exist"
                table_response = _TableResponse({'TableDescription': {'TableName': table_name}})
        except RuntimeError as e:
            return custom_resource_response.failure_response(e.message)
    else:
        raise RuntimeError('Invalid RequestType: {}'.format(request_type))

    return custom_resource_response.success_response(table_response.output, table_response.table_name)


def create_table(table_name, event):
    dynamodb = aws_utils.ClientWrapper(boto3.client('dynamodb'))
    if "GlobalSecondaryIndexes" in event["ResourceProperties"]:
        if "LocalSecondaryIndexes" in event["ResourceProperties"]:
            return dynamodb.create_table(
                AttributeDefinitions=event["ResourceProperties"]["AttributeDefinitions"],
                TableName = table_name,
                KeySchema=event["ResourceProperties"]["KeySchema"],
                ProvisionedThroughput=get_throughput_from_dict(event['ResourceProperties']['ProvisionedThroughput']),
                StreamSpecification=get_stream_spec_from_dict(event["ResourceProperties"].get("StreamSpecification", {})),
                GlobalSecondaryIndexes=sanitize_secondary_indexes(event["ResourceProperties"]["GlobalSecondaryIndexes"]),
                LocalSecondaryIndexes=event["ResourceProperties"]["LocalSecondaryIndexes"]
            )
        else:
            return dynamodb.create_table(
                AttributeDefinitions=event["ResourceProperties"]["AttributeDefinitions"],
                TableName = table_name,
                KeySchema=event["ResourceProperties"]["KeySchema"],
                ProvisionedThroughput=get_throughput_from_dict(event['ResourceProperties']['ProvisionedThroughput']),
                StreamSpecification=get_stream_spec_from_dict(event["ResourceProperties"].get("StreamSpecification", {})),
                GlobalSecondaryIndexes=sanitize_secondary_indexes(event["ResourceProperties"]["GlobalSecondaryIndexes"])
            )

    if "LocalSecondaryIndexes" in event["ResourceProperties"]:
        return dynamodb.create_table(
            AttributeDefinitions=event["ResourceProperties"]["AttributeDefinitions"],
            TableName = table_name,
            KeySchema=event["ResourceProperties"]["KeySchema"],
            ProvisionedThroughput=get_throughput_from_dict(event['ResourceProperties']['ProvisionedThroughput']),
            StreamSpecification=get_stream_spec_from_dict(event["ResourceProperties"].get("StreamSpecification", {})),
            LocalSecondaryIndexes=event["ResourceProperties"]["LocalSecondaryIndexes"]
        )

    return dynamodb.create_table(
        AttributeDefinitions=event["ResourceProperties"]["AttributeDefinitions"],
        TableName = table_name,
        KeySchema=event["ResourceProperties"]["KeySchema"],
        ProvisionedThroughput=get_throughput_from_dict(event['ResourceProperties']['ProvisionedThroughput']),
        StreamSpecification=get_stream_spec_from_dict(event["ResourceProperties"].get("StreamSpecification", {})),
    )


def update_table(table_name, event):
    dynamodb = aws_utils.ClientWrapper(boto3.client('dynamodb'))
    description = dynamodb.describe_table(TableName = table_name)

    update_response = update_throughput(dynamodb, table_name, event, description)

    if update_response:
        wait_for_idle_table(dynamodb, table_name)

    update_response = update_stream_spec(dynamodb, table_name, event, description)

    if update_response:
        wait_for_idle_table(dynamodb, table_name)

    update_global_secondary_indexes(dynamodb, table_name, event, description)

    return { "TableName": table_name }


def update_throughput(dynamodb, table_name, event, description):
    throughput = get_throughput_from_dict(event['ResourceProperties']['ProvisionedThroughput'])
    existing_throughput = get_throughput_from_dict(description['Table']["ProvisionedThroughput"])

    if existing_throughput == throughput:
        return {}

    return dynamodb.update_table(
        AttributeDefinitions=event["ResourceProperties"]["AttributeDefinitions"],
        TableName = table_name,
        ProvisionedThroughput=get_throughput_from_dict(event['ResourceProperties']['ProvisionedThroughput'])
    )


def update_stream_spec(dynamodb, table_name, event, description):
    stream_specification = get_stream_spec_from_dict(event["ResourceProperties"].get("StreamSpecification", {}))
    existing_stream_spec = get_stream_spec_from_dict(description['Table'].get('StreamSpecification', {}))

    if stream_specification == existing_stream_spec:
        return {}

    if stream_specification["StreamEnabled"] and existing_stream_spec["StreamEnabled"]:
        response = dynamodb.update_table(TableName = table_name, StreamSpecification = { "StreamEnabled": False})
        status = response['TableDescription'].get("TableStatus", "")
        wait_for_idle_table(dynamodb, table_name)

    return dynamodb.update_table(
        AttributeDefinitions=event['ResourceProperties']["AttributeDefinitions"],
        TableName = table_name,
        StreamSpecification=stream_specification
    )

def update_global_secondary_indexes(dynamodb, table_name, event, description):
    event_gsi = event["ResourceProperties"].get("GlobalSecondaryIndexes", [])
    existing_gsi = description['Table'].get('GlobalSecondaryIndexes', [])

    gsi_updates = []
    event_index_names = [entry['IndexName'] for entry in event_gsi]
    existing_index_names = [entry['IndexName'] for entry in existing_gsi]

    deleted_indexes = [name for name in existing_index_names if name not in event_index_names]
    for index in deleted_indexes:
        gsi_updates.append({
            'Delete': { 'IndexName': index }
        })

    for index_description in event_gsi:
        if index_description['IndexName'] not in existing_index_names:
            gsi_updates.append({
                'Create': index_description
            })
        else: # possible update
            existing_index_description = [index for index in existing_gsi if index['IndexName'] == index_description['IndexName']][0]
            check_invalid_changes = ['Projection', 'KeySchema']
            for attribute in check_invalid_changes: # you are not allowed to update anything but throughput
                if index_description[attribute] != existing_index_description[attribute]:
                    raise RuntimeError("You cannot change the {} attribute when updating an existing GlobalSecondaryIndex".format(attribute))

            existing_throughput = get_throughput_from_dict(existing_index_description['ProvisionedThroughput'])
            event_throughput = get_throughput_from_dict(index_description['ProvisionedThroughput'])

            if existing_throughput != event_throughput:
                gsi_updates.append({
                    "Update": {
                        "IndexName": index_description['IndexName'],
                        "ProvisionedThroughput": get_throughput_from_dict(index_description['ProvisionedThroughput'])
                    }
                })

    if gsi_updates:
        return dynamodb.update_table(
            TableName = table_name,
            AttributeDefinitions=event['ResourceProperties']["AttributeDefinitions"],
            GlobalSecondaryIndexUpdates = gsi_updates
        )
    return {}


def wait_for_account_tables():
    dynamodb = aws_utils.ClientWrapper(boto3.client('dynamodb'))
    attempts = 0
    while len(updates_in_progress(dynamodb, gather_tables(dynamodb))) > 7:
        backoff(attempts)
        attempts += 1

def get_table_name(event):
    stack_manager = stack_info.StackInfoManager()
    owning_stack_info = stack_manager.get_stack_info(event['StackId'])
    return owning_stack_info.stack_name + '-' + event['LogicalResourceId'] 

def gather_tables(client):
    tables = []
    response = client.list_tables()
    tables = response['TableNames']

    while 'LastEvaluatedTableName' in response:
        response = client.list_tables(ExclusiveStartTableName=response['LastEvaluatedTableName'])
        tables = tables + response['TableNames']

    return tables

def wait_for_idle_table(dynamodb, table_name):
    response = dynamodb.describe_table(TableName = table_name)
    status = response['Table'].get("TableStatus", "")
    attempts = 0
    while status == 'UPDATING':
        backoff(attempts)
        attempts = attempts + 1
        response = dynamodb.describe_table(TableName = table_name)
        status = response['Table'].get("TableStatus", "")

def sanitize_secondary_indexes(index_input):
    sanitized = []
    for index in index_input:
        index["ProvisionedThroughput"] = get_throughput_from_dict(index["ProvisionedThroughput"])
        sanitized.append(index)
    return sanitized

def get_stream_spec_from_dict(input_dict):
    if not input_dict:
        input_dict = {"StreamEnabled": False }
    elif "StreamEnabled" not in input_dict:
        input_dict["StreamEnabled"] = True
    
    return input_dict

def get_throughput_from_dict(input_dict):
    return {
        'ReadCapacityUnits': int(input_dict["ReadCapacityUnits"]),
        'WriteCapacityUnits': int(input_dict["WriteCapacityUnits"])
    }


def updates_in_progress(client, all_tables):
    updating_tables = []
    for table in all_tables:
        try:
            desc_response = client.describe_table(TableName=table)
            if desc_response['Table']['TableStatus'] in ['CREATING', 'UPDATING', 'DELETING']:
                updating_tables.append(table)
        except Exception as e:
            if isinstance(e, ClientError) and e.response['Error']['Code'] in ['ResourceNotFoundException']:
                print "Looking for a table that was already deleted"
            else:
                raise e

    print "{} tables are currently updating".format(len(updating_tables))
    return updating_tables


def backoff(attempts):
    if attempts > MAX_ATTEMPTS:
        raise RuntimeError("Custom::DynamoDB::Table operation has failed. Reached max backoff attempts")

    temp = min(BACKOFF_MAX, BACKOFF_BASE * 2 ** attempts)
    sleep_seconds = temp / 2 + random.uniform(0, temp / 2)
    print 'Backoff attempt {}. Sleeping {} seconds'.format(attempts, sleep_seconds)
    time.sleep(sleep_seconds)
