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
import lex_fixture
import lex_file_util
import datetime
import json
import re
from botocore.exceptions import ClientError

# http://docs.aws.amazon.com/lex/latest/dg/API_PutBot.html
def is_bot_name_legal(bot_name):
    if len(bot_name) < 2:
        return False
    if len(bot_name) > 50:
        return False
    pattern = re.compile("^[a-zA-Z]+((_[a-zA-Z]+)*|([a-zA-Z]+_)*|_)")
    return pattern.match(bot_name)

# http://docs.aws.amazon.com/lex/latest/dg/API_PutBotAlias.html
def is_bot_alias_legal(bot_alias):
    if len(bot_alias) < 1:
        return False
    if len(bot_alias) > 100:
        return False
    pattern = re.compile("^[a-zA-Z$]+((_[a-zA-Z]+)*|([a-zA-Z]+_)*|_)")
    return pattern.match(bot_alias)

# http://docs.aws.amazon.com/lex/latest/dg/API_runtime_PostContent.html
def is_userid_legal(user_id):
    if len(user_id) < 2:
        return False
    if len(user_id) > 100:
        return False
    pattern = re.compile("[0-9a-zA-Z._:-]+")
    return pattern.match(user_id)


def process_bot_desc(bot_desc):
    ret = lex_file_util.bot_desc_file_validator(bot_desc)
    if ret != "":
        return ret
    ret = lex_file_util.create_bot_from_desc(bot_desc)
    return ret

def get_bot_desc(bot_name):
    return lex_file_util.build_bot_desc(bot_name)

def list_bots(next_token):
    response = {}
    # space allows testing from API gateway console, which requires some string input
    if not next_token or len(next_token) <= 1:
        response = boto3.client('lex-models').get_bots()
    else:
        response = boto3.client('lex-models').get_bots(nextToken = next_token)
    ret = {}
    ret['next_token'] = response.get('next_token', '')
    ret['bots'] = []
    for bot in response['bots']:
        botInfo = {}
        botInfo['bot_name'] = bot.get('name', '')
        botInfo['current_version'] = bot.get('version', '')
        botInfo['status'] = bot.get('status', '')
        botInfo['update'] = bot.get('lastUpdateDate', datetime.datetime.now()).strftime("%c")
        botInfo['created'] = bot.get('createdDate', datetime.datetime.now()).strftime("%c")
        ret['bots'].append(botInfo)
    return ret

def get_bot_status(bot_name):
    response = boto3.client('lex-models').get_bot(name = bot_name, versionOrAlias = '$LATEST')
    retStatus = "NORESPONSE"
    if type(response) is dict:
        retStatus = response.get('status', "NOSTATUS")
    return retStatus

def delete_bot(bot_name):
    try: 
        boto3.client('lex-models').delete_bot(name = bot_name)
    except ClientError as e:
        if e.response['Error']['Code'] == 'ResourceInUseException':
            return "INUSE"
        else:   
            return "ERROR: " + e.response['Error']['Code']
    return "DELETED"

def publish_bot(bot_name, bot_version):
    client = boto3.client('lex-models')
    getBotResponse = client.get_bot(name = bot_name, versionOrAlias = '$LATEST')
    if type(getBotResponse) is dict:
        status = getBotResponse.get('status', "NOSTATUS")
        if status != "NOT_BUILT" and status != "READY":
            return status

    if 'intents' in getBotResponse:
        intent_index = 0
        for intent in getBotResponse['intents']:
            intentResponse = client.get_intent(
                name = intent['intentName'],
                version = intent['intentVersion'])
            if 'slots' in intentResponse:
                slot_index = 0
                for slot in intentResponse['slots']:
                    if 'slotTypeVersion' in slot:
                        createSlotTypeResponse = client.create_slot_type_version(name = slot['slotType'])
                        intentResponse['slots'][slot_index]['slotTypeVersion'] = createSlotTypeResponse['version']
                    slot_index += 1
            intentResponse.pop('lastUpdatedDate', None)
            intentResponse.pop('createdDate', None)
            intentResponse.pop('version', None)
            intentResponse.pop('ResponseMetadata', None)
            client.put_intent(**intentResponse)
            createIntentVersionResponse = client.create_intent_version(name = intent['intentName'])
            getBotResponse['intents'][intent_index]['intentVersion'] = createIntentVersionResponse['version']
            intent_index += 1

    getBotResponse.pop('lastUpdatedDate', None)
    getBotResponse.pop('createdDate', None)
    getBotResponse.pop('version', None)
    getBotResponse.pop('status', None)
    getBotResponse.pop('failureReason', None)
    getBotResponse.pop('ResponseMetadata', None)
    client.put_bot(**getBotResponse)
    createBotVersionResponse = client.create_bot_version(name = bot_name)

    # check to see if the bot built version ok before trying to make an alias
    while True:
        status = client.get_bot(name = bot_name, versionOrAlias = createBotVersionResponse['version'])['status']
        if status == 'FAILED':
            return "FAILED"
        elif status == 'READY':
            break

    client.put_bot_alias(
        name = bot_version,
        botVersion = createBotVersionResponse['version'],
        botName = bot_name)
    return "READY"

