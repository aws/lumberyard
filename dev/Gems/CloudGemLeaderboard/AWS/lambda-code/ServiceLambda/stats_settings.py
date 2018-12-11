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

import CloudCanvas
import boto3
import json
import copy
import errors
from decimal import *

STATS_SETTINGS_TABLE = None
LEADERBOARD_INFO_TABLE = None

DEFAULT_SAMPLE_MAX = 500

def __init_table():
    global STATS_SETTINGS_TABLE
    global LEADERBOARD_INFO_TABLE
    if not STATS_SETTINGS_TABLE:
        STATS_SETTINGS_TABLE = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("StatsSettingsTable"))
    if not LEADERBOARD_INFO_TABLE:
        LEADERBOARD_INFO_TABLE = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("LeaderboardInfo"))


def get_leaderboard_stats():
    __init_table()
    stat_defs = []
    try:
        response = STATS_SETTINGS_TABLE.scan()
        for item in response.get("Items", []):
            stat_defs.append(item)

        while "LastEvaluatedKey" in response:
            response = STATS_SETTINGS_TABLE.scan(ExclusiveStartKey=response['LastEvaluatedKey'])
            for item in response.get("Items", []):
                stat_defs.append(item)

    except Exception as e:
        raise Exception("Error fetching stat definitions from table")
    return stat_defs

def get_stat(stat):
    __init_table()
    for stat_def in get_leaderboard_stats():
        if stat_def["name"] == stat:
            return stat_def
    return {}

def add_stat(stat_def):
    __init_table()
    entry = {
        "name": stat_def["name"],
        "mode": stat_def["mode"]
    }
    if "min" in stat_def:
        entry["min"] = Decimal(stat_def["min"]).quantize(
            Decimal('.000000'), rounding=ROUND_HALF_UP)
    if "max" in stat_def:
        entry["max"] = Decimal(stat_def["max"]).quantize(
            Decimal('.000000'), rounding=ROUND_HALF_UP)
    if "sample_size" in stat_def:
        entry["sample_size"] = stat_def["sample_size"]
    try:
        STATS_SETTINGS_TABLE.put_item(Item=entry)
    except Exception as e:
        raise Exception("Error adding new stat definition")

    try:
        with LEADERBOARD_INFO_TABLE.batch_writer() as batch:
            batch.put_item(Item={
                "stat": stat_def["name"],
                "type": "population",
                "value": 0
            })
            batch.put_item(Item={
                "stat": stat_def["name"],
                "type": "max_sample_size",
                "value": stat_def.get("sample_size", DEFAULT_SAMPLE_MAX)
            })
            batch.put_item(Item={
                "stat": stat_def["name"],
                "type": "high_score",
                "value": json.dumps({})
            })
    except Exception as e:
        # attempt to delete STATS_SETTINGS_TABLE entry, because the stat is only half-present in the system
        STATS_SETTINGS_TABLE.delete_item(Key={ "name": stat_def["name"]})
        raise Exception("Error adding new stat definition")


def remove_stat(stat_name):
    __init_table()
    key = {
        "name": stat_name
    }
    try:
        STATS_SETTINGS_TABLE.delete_item(Key=key)
    except Exception as e:
        raise("Error deleting stat definition")


'''
Validates the stat name
Insert your own validation logic here
'''
def is_stat_valid(stat):
    __init_table()
    return stat in [stat_def["name"] for stat_def in get_leaderboard_stats()]
