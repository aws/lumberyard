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

@service.api
def post(request, post_text):
    client = boto3.client('lex-runtime')

    bot_name = post_text.get('bot_name', '')
    if not lex.is_bot_name_legal(bot_name):
        return {'dialog_state' : "ILLEGAL BOT NAME"}
    bot_alias = post_text.get('bot_alias', '')
    if not lex.is_bot_alias_legal(bot_alias):
        return {'dialog_state' : "ILLEGAL BOT ALIAS"}
    user_id = post_text.get('user_id', '')
    if not lex.is_userid_legal(user_id):
        return {'dialog_state' : "ILLEGAL USER ID"}
    
    input_stream = post_text.get('text', '')
    if len(input_stream) < 1 or len(input_stream) > 1024:
        return {'dialog_state' : "INPUT STREAM TOO LONG"}

    response = client.post_text(
        botName = bot_name,
        botAlias = bot_alias,
        userId = user_id,
        inputText = input_stream
    )

    return {
        'intent': response.get('intentName', ''),
        'dialog_state': response.get('dialogState', ''),
        'input_transcript': response.get('inputTranscript', ''),
        'message': response.get('message', ''),
        'session_attributes': response.get('sessionAttributes', ''),
        'slots': str(response.get('slots', '')),
        'slot_to_elicit': str(response.get('slotsToElicit', ''))
    }