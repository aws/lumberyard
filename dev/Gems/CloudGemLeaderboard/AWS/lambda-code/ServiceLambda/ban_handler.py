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
from errors import ClientError
import time
import cgf_lambda_settings
import cgf_service_client
import identity_validator


BAN_TABLE = None
def __init_globals():
    global BAN_TABLE
    if BAN_TABLE:
        return
    try:
        BAN_TABLE = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("BannedPlayerTable"))
    except ValueError as e:
        raise Exception("Error getting table reference")

def __get_player_ban(user):
    global BAN_TABLE
    __init_globals()
    table_key = {
        "user": user
    }
    table_response = BAN_TABLE.get_item(Key=table_key)
    player_ban = table_response.get("Item", {})
    return player_ban

def ban(user):
    global BAN_TABLE
    __init_globals()

    player_ban = __get_player_ban(user)
    if player_ban:
        return "Player {} has already been banned".format(user)

    entry = {
        "user": user,
        "ban_time": time.strftime("%a, %d %b %Y %H:%M:%S +0000", time.gmtime())
    }

    try:
        BAN_TABLE.put_item(Item=entry)
    except Exception as e:
        raise Exception('Error creating a new ban')

    return "Ban of {} succeeded".format(user)

def lift_ban(user):
    global BAN_TABLE
    __init_globals()
    player_ban = __get_player_ban(user)

    if not player_ban:
        return "Player {} is not banned, no operation to perform".format(user)

    table_key = {
        "user": user
    }
    try:
        response = BAN_TABLE.delete_item(Key=table_key)
    except ClientError as e:
        raise ClientError("Unban operation failed")

    return "Ban of {} has been lifted".format(user)

def is_player_banned(user):
    global BAN_TABLE
    interface_url = cgf_lambda_settings.get_service_url(
        "CloudGemPlayerAccount_banplayer_1_0_0")
    if interface_url:
        return check_player_account_gem_for_ban(interface_url, user)
    __init_globals
    player_ban = __get_player_ban(user)
    if not player_ban:
        return False
    return True

def get_banned_players():
    __init_globals()
    banned_players = []
    response = BAN_TABLE.scan()
    for item in response.get("Items", []):
        banned_players.append(item["user"])
    while "LastEvaluatedKey" in response:
        response = BAN_TABLE.scan(ExclusiveStartKey=response["LastEvaluatedKey"])
        for item in response.get("Items", []):
            banned_players.append(item["user"])
    return banned_players


def check_player_account_gem_for_ban(interface_url, user):
    # get cognito id from identity map
    cognito_id = identity_validator.get_id_from_user(user)
    client = cgf_service_client.for_url(
        interface_url, verbose=True, session=boto3._get_default_session())
    result = client.navigate('accountinfo', cognito_id).GET()
    # ask player account if that player is banned
    return result.DATA.get('AccountBlacklisted', False)
