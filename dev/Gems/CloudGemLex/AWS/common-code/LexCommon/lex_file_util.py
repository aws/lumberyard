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
        name = ""
        if not 'name' in intent:
            ret += "INTENT name MISSING; "
        else:
            name = intent['name']

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
            client.put_slot_type(**slot_type)

    #iterate intents and put_intent
    for intent in bot_desc['intents']:
        client.put_intent(**intent)

    #put_bot
    client.put_bot(**bot_desc['bot'])
    return "ACCEPTED"

def build_bot_desc(bot_name):
    client = boto3.client('lex-models')
    bot = client.get_bot(
        name = bot_name,
        versionOrAlias="$LATEST"
    )
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
    for intent_in_bot in bot['intents']:
        # if intentVersion missing, it's a built in intent
        if 'intentVersion' in intent_in_bot:
            intent = client.get_intent(
                name = intent_in_bot['intentName'],
                version = intent_in_bot['intentVersion']
                )
            intent.pop('lastUpdatedDate', None)
            intent.pop('createdDate', None)
            intent.pop('checksum', None)
            intent.pop('version', None)
            intent.pop('ResponseMetadata', None)
            intents.append(intent)

    slot_types = []
    for intent in intents:
        for slot in intent['slots']:
            if not any(slot_type['name'] == slot['slotType'] for slot_type in slot_types):
                #if slotVersion missing, it's a built in slot type
                if 'slotTypeVersion' in slot:
                    slot_type = client.get_slot_type(
                        name = slot['slotType'],
                        version = slot['slotTypeVersion'])
                    slot_type.pop('lastUpdatedDate', None)
                    slot_type.pop('createdDate', None)
                    slot_type.pop('checksum', None)
                    slot_type.pop('version', None)
                    slot_type.pop('ResponseMetadata', None)
                    slot_types.append(slot_type)

    ret = {
        'bot' : bot,
        'intents' : intents,
        'slot_types' : slot_types
    }
    return ret
