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

import service
import boto3
import lex
import json

@service.api
def post(request, post_text):
    client = boto3.client('lex-runtime')

    name = post_text.get('name', '')
    if not lex.is_bot_name_legal(name):
        return {'dialog_state' : "ILLEGAL BOT NAME"}
    bot_alias = post_text.get('bot_alias', '')
    if not lex.is_bot_alias_legal(bot_alias):
        return {'dialog_state' : "ILLEGAL BOT ALIAS"}
    user_id = post_text.get('user_id', '')
    if not lex.is_userid_legal(user_id):
        return {'dialog_state' : "ILLEGAL USER ID"}
    
    session_attributes = {}
    session_attr_json = post_text.get('session_attributes', '')
    if len(session_attr_json) > 0:
        session_attributes = json.loads(session_attr_json)

    input_stream = post_text.get('text', '')
    if len(input_stream) < 1 or len(input_stream) > 1024:
        return {'dialog_state' : "INPUT STREAM TOO LONG"}

    try:
        response = client.post_text(
            botName = name,
            botAlias = bot_alias,
            userId = user_id,
            sessionAttributes = session_attributes,
            inputText = input_stream
        )
    except Exception as e:
        raise Exception(str(e))

    response_session_attributes = response.get('sessionAttributes', '')
    if response_session_attributes != '':
        response_session_attributes = json.dumps(response_session_attributes)

    response_slots = response.get('slots', '')
    if response_slots != '':
        response_slots = json.dumps(response_slots)

    return {
        'intent': response.get('intentName', ''),
        'dialog_state': response.get('dialogState', ''),
        'input_transcript': response.get('inputTranscript', ''),
        'message': response.get('message', ''),
        'session_attributes': response_session_attributes,
        'slots': response_slots,
        'slot_to_elicit': str(response.get('slotsToElicit', ''))
    }