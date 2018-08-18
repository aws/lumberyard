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
import uuid
from botocore.exceptions import ClientError
from botocore.exceptions import ParamValidationError

# http://docs.aws.amazon.com/lex/latest/dg/API_PutBot.html
def is_bot_name_legal(name):
    if len(name) < 2:
        return False
    if len(name) > 50:
        return False
    pattern = re.compile("^[a-zA-Z]+((_[a-zA-Z]+)*|([a-zA-Z]+_)*|_)")
    return pattern.match(name)

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

def get_bot_desc(name, version):
    return lex_file_util.build_bot_desc(name, version)

def list_bots(next_token):
    response = {}
    try:
        # space allows testing from API gateway console, which requires some string input
        if not next_token or len(next_token) <= 1:
            response = boto3.client('lex-models').get_bots()
        else:
            response = boto3.client('lex-models').get_bots(nextToken = next_token)
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    ret = retrive_items_from_response(response, 'bots')
    return ret

def retrive_items_from_response(response, type):
    ret = {}
    ret['nextToken'] = response.get('nextToken', '')
    ret[type] = []
    for item in response[type]:
        item_section = {}
        item_section['name'] = item.get('name', '')
        item_section['version'] = item.get('version', '')
        if type == 'bots':
            item_section['status'] = item.get('status', '')
        item_section['update'] = item.get('lastUpdateDate', datetime.datetime.now()).strftime("%c")
        item_section['created'] = item.get('createdDate', datetime.datetime.now()).strftime("%c")
        ret[type].append(item_section)
    return ret

def get_bot_count():
    count = 0
    response = boto3.client('lex-models').get_bots()
    count += len(response['bots'])
    next_token = response.get('nextToken', '')
    while next_token:
        response = boto3.client('lex-models').get_bots(nextToken = next_token)
        count += response['bots']
        next_token = response.get('nextToken', '')
    return count

def get_bot_status(name):
    try:
        response = boto3.client('lex-models').get_bot(name = name, versionOrAlias = '$LATEST')
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    retStatus = "NORESPONSE"
    if type(response) is dict:
        retStatus = response.get('status', "NOSTATUS")
    return retStatus

def get_bot(name, version = "$LATEST"):
    client = boto3.client('lex-models')
    bot = {}
    try:
        bot = client.get_bot(
            name = name,
            versionOrAlias = version
        )
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}  
    #pop removes the key from the dict if it exists
    #these are all values not needed for creating or updating
    #so they are removed for sending back to client
    bot.pop('status', None)
    bot.pop('failureReason', None)
    bot.pop('lastUpdatedDate', None)
    bot.pop('createdDate', None)
    bot.pop('ResponseMetadata', None)
    return bot

def put_bot(bot_section):
    client = boto3.client('lex-models')
    try:
        put_bot_response = client.put_bot(**bot_section['bot'])
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    except ParamValidationError as e:
            return "ERROR: Unknown parameter in input."
    return "ACCEPTED"

def delete_bot(name, version = None):
    try:
        if version:
            boto3.client('lex-models').delete_bot_version(name = name, version = version)
        else:
            boto3.client('lex-models').delete_bot(name = name)
    except ClientError as e:
        if e.response['Error']['Code'] == 'ResourceInUseException':
            return "INUSE"
        else:
            return "ERROR: " + e.response['Error']['Message']
    return "DELETED"

def create_bot_version(name):
    client = boto3.client('lex-models')
    create_bot_version_response = client.create_bot_version(name = name)
    return create_bot_version_response.get('status', 'NO RESPONSE')

def list_bot_versions(name, next_token):
    response = {}
    try:
        # space allows testing from API gateway console, which requires some string input
        if not next_token or len(next_token) <= 1:
            response = boto3.client('lex-models').get_bot_versions(name = name)
        else:
            response = boto3.client('lex-models').get_bot_versions(name = name, nextToken = next_token)
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    ret = retrive_items_from_response(response, 'bots')
    return ret

def list_builtin_intents(next_token):
    response = {}
    try:
        # space allows testing from API gateway console, which requires some string input
        if not next_token or len(next_token) <= 1:
            response = boto3.client('lex-models').get_builtin_intents()
        else:
            response = boto3.client('lex-models').get_builtin_intents(
                nextToken = next_token
            )
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    ret = {}
    ret['nextToken'] = response.get('nextToken', '')
    ret['intents'] = []
    for intent in response.get('intents', []):
        intent_section = {}
        intent_section['name'] = intent.get('signature', '')
        ret['intents'].append(intent_section)
    return ret

def list_custom_intents(next_token):
    response = {}
    try:
        # space allows testing from API gateway console, which requires some string input
        if not next_token or len(next_token) <= 1:
            response = boto3.client('lex-models').get_intents()
        else:
            response = boto3.client('lex-models').get_intents(nextToken = next_token)
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    ret = retrive_items_from_response(response, 'intents')
    return ret

def get_builtin_intent(name):
    client = boto3.client('lex-models')
    try:
        intent = client.get_builtin_intent(
            signature = name)
        intent['name'] = intent.get('signature', "")
        intent.pop('signature', None)
        intent.pop('supportedLocales', None)
        intent.pop('ResponseMetadata', None)
        return intent
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

def get_custom_intent(name, version = "$LATEST"):
    client = boto3.client('lex-models')
    intent = {}
    try:
        intent = client.get_intent(
            name = name,
            version = version)
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}
    #pop removes the key from the dict if it exists
    #these are all values not needed for creating or updating
    #so they are removed for sending back to client
    intent.pop('lastUpdatedDate', None)
    intent.pop('createdDate', None)
    intent.pop('ResponseMetadata', None)
    return intent

def put_intent(intent_section):
    if intent_section['intent'].get('dialogCodeHook', {}).get('uri', ''):
        __add_lambda_invoke_permission(intent_section['intent']['dialogCodeHook']['uri'])    
    if intent_section['intent'].get('fulfillmentActivity', {}).get('type', '') == 'CodeHook':
         __add_lambda_invoke_permission(intent_section['intent']['fulfillmentActivity']['codeHook']['uri'])

    client = boto3.client('lex-models')
    try:
        put_intent_response = client.put_intent(**intent_section['intent'])
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    except ParamValidationError as e:
            return "ERROR: Unknown parameter in input."

    return "ACCEPTED"

def delete_intent(name, version = None):
    try:
        if version:
            boto3.client('lex-models').delete_intent_version(name = name, version = version)
        else:
            boto3.client('lex-models').delete_intent(name = name)
    except ClientError as e:
        if e.response['Error']['Code'] == 'ResourceInUseException':
            return "INUSE"
        else:
            return "ERROR: " + e.response['Error']['Message']
    return "DELETED"

def create_intent_version(name):
    client = boto3.client('lex-models')
    try:
        create_intent_version_response = client.create_intent_version(name = name)
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    return "ACCEPTED"

def list_intent_versions(name, next_token):
    response = {}
    try:
        # space allows testing from API gateway console, which requires some string input
        if not next_token or len(next_token) <= 1:
            response = boto3.client('lex-models').get_intent_versions(name = name)
        else:
            response = boto3.client('lex-models').get_intent_versions(name = name, nextToken = next_token)
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    ret = retrive_items_from_response(response, 'intents')
    return ret

def list_builtin_slot_types(next_token):
    response = {}
    try:
        # space allows testing from API gateway console, which requires some string input
        if not next_token or len(next_token) <= 1:
            response = boto3.client('lex-models').get_builtin_slot_types()
        else:
            response = boto3.client('lex-models').get_builtin_slot_types(
                nextToken = next_token
            )
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    ret = {}
    ret['nextToken'] = response.get('nextToken', '')
    ret['slotTypes'] = []
    for slot_type in response['slotTypes']:
        slot_type_section = {}
        slot_type_section['name'] = slot_type.get('signature', '')
        ret['slotTypes'].append(slot_type_section)
    return ret

def list_custom_slot_types(next_token):
    response = {}
    try:
        # space allows testing from API gateway console, which requires some string input
        if not next_token or len(next_token) <= 1:
            response = boto3.client('lex-models').get_slot_types()
        else:
            response = boto3.client('lex-models').get_slot_types(nextToken = next_token)
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    ret = retrive_items_from_response(response, 'slotTypes')
    return ret

def get_slot_type(name, version = "$LATEST"):
    client = boto3.client('lex-models')
    slot_type = {}
    try:
        slot_type = client.get_slot_type(
            name = name,
            version = version)
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    #pop removes the key from the dict if it exists
    #these are all values not needed for creating or updating
    #so they are removed for sending back to client
    slot_type.pop('lastUpdatedDate', None)
    slot_type.pop('createdDate', None)
    slot_type.pop('ResponseMetadata', None)
    return slot_type

def put_slot_type(slot_type_section):
    client = boto3.client('lex-models')
    try:
        put_slot_type_response = client.put_slot_type(**slot_type_section['slotType'])
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    except ParamValidationError as e:
            return "ERROR: Unknown parameter in input."
    return "ACCEPTED"

def delete_slot_type(name, version = None):
    try:
        if version:
            boto3.client('lex-models').delete_slot_type_version(name = name, version = version)
        else:
            boto3.client('lex-models').delete_slot_type(name = name)
    except ClientError as e:
        if e.response['Error']['Code'] == 'ResourceInUseException':
            return "INUSE"
        else:
            return "ERROR: " + e.response['Error']['Message']
    return "DELETED"

def create_slot_type_version(name):
    client = boto3.client('lex-models')
    try:
        create_slot_type_version_response = client.create_slot_type_version(name = name)
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    return "ACCEPTED"

def list_slot_type_versions(name, next_token):
    response = {}
    try:
        # space allows testing from API gateway console, which requires some string input
        if not next_token or len(next_token) <= 1:
            response = boto3.client('lex-models').get_slot_type_versions(name = name)
        else:
            response = boto3.client('lex-models').get_slot_type_versions(name = name, nextToken = next_token)
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    ret = retrive_items_from_response(response, 'slotTypes')
    return ret

def list_bot_aliases(name, next_token):
    response = {}
    try:
        # space allows testing from API gateway console, which requires some string input
        if not next_token or len(next_token) <= 1:
            response = boto3.client('lex-models').get_bot_aliases(
                botName = name
            )
        else:
            response = boto3.client('lex-models').get_bot_aliases(
                botName=name,
                nextToken = next_token
            )
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    ret = {}
    ret['nextToken'] = response.get('nextToken', '')
    ret['aliases'] = []
    for alias in response['BotAliases']:
        alias_info = {}
        alias_info['name'] = alias.get('name', '')
        alias_info['botVersion'] = alias.get('botVersion', "")
        alias_info['checksum'] = alias.get('checksum', "")
        ret['aliases'].append(alias_info)
    return ret

def get_bot_alias(name, bot_name):
    alias_response = {}
    try:
        alias_response = boto3.client('lex-models').get_bot_alias(
            name = name,
            botName = bot_name
        )
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}

    alias_response.pop('lastUpdatedDate', None)
    alias_response.pop('createdDate', None)
    alias_response.pop('ResponseMetadata', None)
    return alias_response

def put_bot_alias(alias_info):
    client = boto3.client('lex-models')
    try:
        put_bot_alias_response = client.put_bot_alias(**alias_info['alias'])
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    except ParamValidationError as e:
            return "ERROR: Unknown parameter in input."
    return "ACCEPTED"

def delete_bot_alias(name, bot_name):
    try: 
        boto3.client('lex-models').delete_bot_alias(name = name, botName = bot_name)
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    return "DELETED"

def get_intent_dependency():
    intent_dependency = {}

    ret = list_bots('e')
    bots = ret['bots']
    while ret['nextToken']:
        ret = list_bots(ret['nextToken'])
        bots = bots + ret['bots']

    bot_versions = []
    for bot in bots:
        ret['nextToken'] = 'e'
        while ret['nextToken']:
            ret = list_bot_versions(bot['name'], ret['nextToken'])
            bot_versions = bot_versions + ret['bots']
    
    for bot_version in bot_versions:
        bot = get_bot(bot_version['name'], bot_version['version'])
        for intent in bot.get('intents', []):
            if (intent['intentVersion']):
                latest_intent = get_custom_intent(name = intent['intentName'], version = '$LATEST')
                if ('error' in latest_intent):
                    return latest_intent['error']
                current_intent = get_custom_intent(name = intent['intentName'], version = intent['intentVersion'])
                if ('error' in current_intent):
                    return current_intent['error']
                if latest_intent['checksum'] == current_intent['checksum'] and current_intent['version'] != '$LATEST':
                    add_to_intent_dependency(intent['intentName'], '$LATEST', bot_version, intent_dependency)
                add_to_intent_dependency(intent['intentName'], intent['intentVersion'], bot_version, intent_dependency)

    return intent_dependency

def add_to_intent_dependency(intent_name, intent_version, bot_version, intent_dependency):
    if not intent_name in intent_dependency:
        intent_dependency[intent_name] = {str(intent_version): [bot_version]}
    elif not intent_version in intent_dependency[intent_name]:
        intent_dependency[intent_name][str(intent_version)] = [bot_version]
    else:
        intent_dependency[intent_name][str(intent_version)].append(bot_version)

def publish_bot(name, version):
    client = boto3.client('lex-models')
    build_bot(name)
    createBotVersionResponse = {}
    try:
        createBotVersionResponse = client.create_bot_version(name = name)
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    # check to see if the bot built version ok before trying to make an alias
    while True:
        status = client.get_bot(name = name, versionOrAlias = createBotVersionResponse['version'])['status']
        if status == 'FAILED':
            return "FAILED"
        elif status == 'READY':
            break

    aliasChecksum = ''
    try:
        response = client.get_bot_alias(
            name = version,
            botName = name)
        aliasChecksum = response.get('checksum', '')
    except ClientError as e:
        if e.response['Error']['Code'] != 'NotFoundException':
            return "FAILED"

    if aliasChecksum:
        client.put_bot_alias(
            name = version,
            botVersion = createBotVersionResponse['version'],
            botName = name,
            checksum = aliasChecksum)
    else:
        client.put_bot_alias(
            name = version,
            botVersion = createBotVersionResponse['version'],
            botName = name)

    return "READY"

def build_bot(name):
    client = boto3.client('lex-models')
    getBotResponse = {}
    try:
        getBotResponse = client.get_bot(name = name, versionOrAlias = '$LATEST')
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']

    if type(getBotResponse) is dict:
        status = getBotResponse.get('status', "NOSTATUS")
        if status != "NOT_BUILT" and status != "READY":
            return status

    if 'intents' in getBotResponse:
        intent_index = 0
        for intent in getBotResponse['intents']:
            intentResponse = {}
            try:
                intentResponse = client.get_intent(
                    name = intent['intentName'],
                    version = intent['intentVersion'])
            except ClientError as e:
                return "ERROR: " + e.response['Error']['Message']
            if intent['intentVersion'] == '$LATEST':
                if 'slots' in intentResponse:
                    slot_index = 0
                    for slot in intentResponse['slots']:
                        if ('slotTypeVersion' in slot) and (slot['slotTypeVersion'] == '$LATEST'):
                            createSlotTypeResponse = {}
                            try:
                                createSlotTypeResponse = client.create_slot_type_version(name = slot['slotType'])
                            except ClientError as e:
                                return "ERROR: " + e.response['Error']['Message']
                            intentResponse['slots'][slot_index]['slotTypeVersion'] = createSlotTypeResponse['version']
                        slot_index += 1
                intentResponse.pop('lastUpdatedDate', None)
                intentResponse.pop('createdDate', None)
                intentResponse.pop('version', None)
                intentResponse.pop('ResponseMetadata', None)
                ret = lex_file_util.validate_intents_section([intentResponse])
                if ret:
                    return "ERROR: " + ret
                try:
                    client.put_intent(**intentResponse)
                except ClientError as e:
                    return "ERROR: " + e.response['Error']['Message']
                except ParamValidationError as e:
                    return "ERROR: Unknown parameter in input."
                createIntentVersionResponse = {}
                try:
                    createIntentVersionResponse = client.create_intent_version(name = intent['intentName'])
                except ClientError as e:
                    return "ERROR: " + e.response['Error']['Message']
                getBotResponse['intents'][intent_index]['intentVersion'] = createIntentVersionResponse['version']
            intent_index += 1

    getBotResponse.pop('lastUpdatedDate', None)
    getBotResponse.pop('createdDate', None)
    getBotResponse.pop('version', None)
    getBotResponse.pop('status', None)
    getBotResponse.pop('failureReason', None)
    getBotResponse.pop('ResponseMetadata', None)
    getBotResponse['processBehavior'] = 'BUILD'
    try:
        client.put_bot(**getBotResponse)
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    except ParamValidationError as e:
            return "ERROR: Unknown parameter in input."
    return "READY"

def __add_lambda_invoke_permission(function_name):
    client = boto3.client('lambda')
    client.add_permission(
        Action='lambda:InvokeFunction',
        FunctionName=function_name,
        Principal='lex.amazonaws.com',
        StatementId=str(uuid.uuid4()),
    )