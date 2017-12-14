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
import base64
import lex
import json

@service.api
def post(request, post_audio):
    client = boto3.client('lex-runtime')
    
    name = post_audio.get('name', '')
    if not lex.is_bot_name_legal(name):
        return {'dialog_state' : "ILLEGAL BOT NAME"}
    bot_alias = post_audio.get('bot_alias', '')
    if not lex.is_bot_alias_legal(bot_alias):
        return {'dialog_state' : "ILLEGAL BOT ALIAS"}
    user_id = post_audio.get('user_id', '')
    if not lex.is_userid_legal(user_id):
        return {'dialog_state' : "ILLEGAL USER ID"}

    session_attributes = {}
    session_attr_json = post_audio.get('session_attributes', '')
    if len(session_attr_json) > 0:
        session_attributes = json.loads(session_attr_json)

    inputBytes = base64.b64decode(post_audio.get('audio', ''))    
    response = client.post_content(
        botName = name,
        botAlias = bot_alias,
        userId = user_id,
        sessionAttributes = session_attributes,
        inputStream = inputBytes,
        contentType = 'audio/l16; rate=16000; channels=1',
        accept = 'text/plain; charset=utf-8'
    )

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