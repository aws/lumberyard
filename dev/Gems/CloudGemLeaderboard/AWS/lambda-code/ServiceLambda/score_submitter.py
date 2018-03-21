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
import json
import CloudCanvas
import leaderboard_utils
import identity_validator
import stats_settings
from decimal import *

import errors

REQUIRED_FIELDS = ["user", "stat", "value"]

'''
Score data should be of the form:
{
    'user' : String,   <-- The User to associate with the score
    'stat' : String,   <-- The name of the stat to associate with the score
    'value' : Number,    <-- The numeric value of the score
}
'''
def validate_request_data(score_data, cognito_id):
    # make sure we have all required fields
    missing_fields = []
    for field in REQUIRED_FIELDS:
        if field not in score_data:
            missing_fields.append(field)
    if missing_fields:
        print("Request is missing the following required fields in ScoreData ({})".format(missing_fields.join(", ")))
        raise errors.ClientError("Request is missing required fields")

    if not stats_settings.is_stat_valid(score_data["stat"]):
        print("Client provided stat ({}) is not valid".format(score_data["stat"]))
        if len(stats_settings.get_leaderboard_stats()) == 0:
            raise errors.ClientError("There are no stats on this leaderboard. Go to the Cloud Gem Portal to add a leaderboard")
        raise errors.ClientError("Invalid stat")

    if not leaderboard_utils.is_score_valid(score_data["value"], score_data["stat"]):
        print("Provided score ({}) is invalid for stat ({})".format(score_data["value"], score_data["stat"]))
        raise errors.ClientError("Invalid score")

    if not identity_validator.validate_user(score_data["user"], cognito_id):
        print("The user {} provided incorrect cognito ID {}".format(score_data["user"], cognito_id))
        raise errors.ClientError("This client is not authorized to submit scores for user {}".format(score_data["user"]))

    if not leaderboard_utils.is_user_valid(score_data["user"]):
        print("The user ({}) is invalid for stat ({})".format(score_data["user"], score_data["stat"]))
        raise errors.ClientError("Invalid user")



def update_table_entry(key, submission, existing_data, table):    
    final_entry = leaderboard_utils.resolve_update(submission, existing_data)
    try:        
        table.put_item(Item=final_entry)
        # cut down to just the one you are
        return {
            "user": submission["user"],
            "stat": submission["stat"],
            "value": final_entry[submission["stat"]]
        }
    except Exception as e:
        raise Exception('Error updating table. Error=>{}'.format(e))


def create_table_entry(key, data, table):
    print("creating new entry {} with {}".format(key, data))
    if " " in key:
        raise errors.ClientError("No Spaces are allowed in username")
    entry = {}
    entry["user"] = key
    entry[data["stat"]] = data["value"]
    try:
        table.put_item(Item=entry)
    except Exception as e:
        raise Exception('Error creating a new table. Error=>{}'.format(e))


def delete_score(user, stat):
    table_key = {
        "user" : user
    }
    try:
        main_table = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("MainTable"))
    except ValueError as e:
        raise Exception("Error getting table reference")

    response = main_table.get_item(Key=table_key)
    existing_player_scores = response.get("Item", {})

    if not stat in existing_player_scores:
        return

    # delete from main table
    del existing_player_scores[stat]
    try:
        main_table.put_item(Item=existing_player_scores)
    except Exception as e:
        raise Exception('Error creating a new table entry=>{}'.format(e))

    # delete from sample
    try:
        leaderboard_info_table = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("LeaderboardInfo"))
        leaderboard_table = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("LeaderboardTable"))
    except Exception as e:
        raise Exception("Error getting table references")

    try:
        key={"type": "{}_sample".format(stat), "user": user}
        response = leaderboard_table.get_item(Key=key)
        if response.get("Item", {}):
            leaderboard_table.delete_item(Key=key)

        info = {"stat": stat, "type": "population"}
        response = leaderboard_info_table.get_item(Key = info)
        if response.get("Item", {}):
            population = response["Item"]["value"] - 1
            info["value"] = population
            leaderboard_info_table.put_item(Item = info)
        else:
            print("WARNING: there is no population info for {}".format(stat))
    except Exception as e:
        raise Exception("Error reading from leaderboard sample or info table")





def submit_score(score_data, cognito_id):
    validate_request_data(score_data, cognito_id)
    # make sure that the score is a decimal, not a float and round the decimal as DynamoDb doesn't support double decimals        
    score_data["value"] = Decimal(Decimal(score_data["value"]).quantize(Decimal('.000000'), rounding=ROUND_HALF_UP))        
    # read current data of the table
    table_key = {
        "user" : score_data["user"]
    }
    try:
        main_table = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("MainTable"))
    except ValueError as e:
        raise Exception("Error getting table reference")
    response = main_table.get_item(Key=table_key)
    existing_player_scores = response.get("Item", {})

    # submit the score
    if existing_player_scores:
        return update_table_entry(score_data["user"], score_data, existing_player_scores, main_table)

    create_table_entry(score_data["user"], score_data, main_table)
    return score_data
