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
from botocore.exceptions import ClientError
from botocore.exceptions import ParamValidationError

def vaidate_bot_section(bot):
    #TODO: clarificationPrompt
    #TODO: abortStatement
    ret = ""
    if not 'name' in bot:
        ret += "BOT name MISSING; "
    if not 'intents' in bot:
        ret += "BOT intents MISSING; "
    if not 'locale' in bot:
        ret += "BOT locale MISSING; "
    if not 'childDirected' in bot:
        ret += "BOT childDirected MISSING; "
    return ret

def validate_intents_section(intents):
    ret = ""
    for intent in intents:
        if not 'name' in intent:
            ret += "INTENT name MISSING; "
        elif not 'parentIntentSignature' in intent:
            if not 'sampleUtterances' in intent or len(intent['sampleUtterances']) == 0:
                ret += "Custom intent " +intent['name'] + " needs at least one sample utterance; "

    #TODO some more specific checking
    #BUT should check if Lex API does this checking and returns useful error?
    return ret

def validate_slot_type_section(slot_types):
    #TODO check enum valus or see if Lex API returns useful errors
    ret = ""
    for slot_type in slot_types:
        if not 'name' in slot_type:
            ret += "SLOT TYPE name MISSING"

    return ret

def bot_desc_file_validator(bot_desc):
    # check for the required sections
    ret = ""
    if not 'bot' in bot_desc:
        ret += "BOT KEY MISSING; "
    else:
        ret += vaidate_bot_section(bot_desc['bot'])

    if not 'intents' in bot_desc:
        ret += "INTENT KEY MISSING; "
    else:
        ret += validate_intents_section(bot_desc['intents'])

    if 'slot_types' in bot_desc:
        ret += validate_slot_type_section(bot_desc['slot_types'])

    return ret

def create_bot_from_desc(bot_desc):
    client = boto3.client('lex-models')
    #iterate slot_types and put_slot_type
    if 'slot_types' in bot_desc:
        for slot_type in bot_desc['slot_types']:
            current_slot_type = {}
            try:
                current_slot_type = client.get_slot_type(name = slot_type['name'], version = '$LATEST')
                slot_type['checksum'] = current_slot_type['checksum']
            except ClientError as e:
                if e.response['Error']['Code'] != 'NotFoundException':
                    return "ERROR: " + e.response['Error']['Message']
            try:
                client.put_slot_type(**slot_type)
            except ClientError as e:
                return "ERROR: " + e.response['Error']['Message']
            except ParamValidationError as e:
                return "ERROR: Unknown parameter in input."

    #iterate intents and put_intent
    for intent in bot_desc['intents']:
        current_intent = {}
        try:
            current_intent = client.get_intent(name = intent['name'], version = '$LATEST')
            intent['checksum'] = current_intent['checksum']
        except ClientError as e:
            if e.response['Error']['Code'] != 'NotFoundException':
                return "ERROR: " + e.response['Error']['Message']
        try:
            client.put_intent(**intent)
        except ClientError as e:
            return "ERROR: " + e.response['Error']['Message']
        except ParamValidationError as e:
            return "ERROR: Unknown parameter in input."

    #put_bot
    current_bot = {}
    try:
        current_bot = client.get_bot(name = bot_desc['bot']['name'], versionOrAlias = '$LATEST')
        bot_desc['bot']['checksum'] = current_bot['checksum']
    except ClientError as e:
        if e.response['Error']['Code'] != 'NotFoundException':
            return "ERROR: " + e.response['Error']['Message']
    try:
        client.put_bot(**bot_desc['bot'])
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    return "ACCEPTED"

def build_bot_desc(name, version):
    client = boto3.client('lex-models')
    bot = {}
    try:
        bot = client.get_bot(
            name = name,
            versionOrAlias= version
        )
    except ClientError as e:
        return {'error': "ERROR: " + e.response['Error']['Message']}
    #pop removes the key from the dict if it exists
    #these are all values not needed for creating
    #so they are removed for sending back to client
    bot.pop('status', None)
    bot.pop('failureReason', None)
    bot.pop('lastUpdatedDate', None)
    bot.pop('createdDate', None)
    bot.pop('checksum', None)
    bot.pop('version', None)
    bot.pop('ResponseMetadata', None)

    intents = []
    for index, intent_in_bot in enumerate(bot.get('intents', [])):
        # if intentVersion missing, it's a built in intent
        if 'intentVersion' in intent_in_bot:
            intent = {}
            try:
                intent = client.get_intent(
                    name = intent_in_bot['intentName'],
                    version = intent_in_bot['intentVersion']
                    )
            except ClientError as e:
                return {'error': "ERROR: " + e.response['Error']['Message']}

            intent.pop('lastUpdatedDate', None)
            intent.pop('createdDate', None)
            intent.pop('checksum', None)
            intent.pop('version', None)
            intent.pop('ResponseMetadata', None)
            intents.append(intent)

            bot['intents'][index]['intentVersion'] = '$LATEST'

    slot_types = []
    for intent in intents:
        for index, slot in enumerate(intent.get('slots', [])):
            if not any(slot_type['name'] == slot['slotType'] for slot_type in slot_types):
                #if slotVersion missing, it's a built in slot type
                if 'slotTypeVersion' in slot:
                    slot_type = {}
                    try:
                        slot_type = client.get_slot_type(
                            name = slot['slotType'],
                            version = slot['slotTypeVersion'])
                    except ClientError as e:
                        return {'error': "ERROR: " + e.response['Error']['Message']}

                    slot_type.pop('lastUpdatedDate', None)
                    slot_type.pop('createdDate', None)
                    slot_type.pop('checksum', None)
                    slot_type.pop('version', None)
                    slot_type.pop('ResponseMetadata', None)
                    slot_types.append(slot_type)

                    intent['slots'][index]['slotTypeVersion'] = '$LATEST'

    ret = {
        'bot' : bot,
        'intents' : intents,
        'slot_types' : slot_types
    }
    return ret