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
import CloudCanvas

from errors import ClientError

SPEECH_TABLE = None

def __get_table():
    global SPEECH_TABLE
    if SPEECH_TABLE == None:
        SPEECH_TABLE = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("SpeechLibTable"))
    return SPEECH_TABLE

def __get_all_speech_entries():
    entries = []
    response = __get_table().scan()
    entries = response.get("Items", [])
    while "LastEvaluatedKey" in response:
        response = self.__get_table().scan(ExclusiveStartKey=response['LastEvaluatedKey'])
        entries += response.get("Items", [])
    return entries

def get_speech_lib(tags = []):
    entries = __get_all_speech_entries()
    exclude = []
    for tag in tags:
        entries = [e for e in entries if tag in e["tags"]]
    return entries

def add_speech_entry(speech):
    status = ''
    key = { "character": speech["character"], "line": speech["line"] }
    if __get_table().get_item(Key=key).get("Item", {}):
        status = 'duplicate'
    else:
        __get_table().put_item(Item=speech)
        status = 'success'
    return status

def delete_speech_entry(speech):
    key = { "character": speech["character"], "line": speech["line"] }
    __get_table().delete_item(Key=key)

def import_speeches(speechEntries):
    for speech in speechEntries:
        add_speech_entry(speech)