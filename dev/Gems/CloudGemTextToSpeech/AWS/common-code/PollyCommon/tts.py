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
import botocore
import CloudCanvas
import hashlib
import json

from botocore.exceptions import ClientError

BUCKET = {'ttscache': None, 'packagedvoicelines': None}
TTSCACHE = 'ttscache'

RUNTIME_CAPABILITIES_FLAG_KEY = 'disableRuntimeCapabilitiesFlag'
RUNTIME_CACHING_FLAG_KEY = 'disableRuntimeCachingFlag'

def get_speech_marks(tts_info, from_cgp = False):
    '''Returns the url of the speech marks file for the given string/voice'''
    if not (from_cgp or runtime_capabilities_enabled()):
        return ""

    if len(tts_info["voice"]) == 0:
        raise ClientError("Invalid request, Voice parameter was empty")

    tts_key = "{}-{}.txt".format(generate_key(tts_info), tts_info["speechMarks"])
    if key_exists(tts_key, TTSCACHE):
        return generate_presigned_url(tts_key)

    speech_mark_types = __get_speech_mark_types(tts_info['speechMarks'])

    polly_client = boto3.client("polly")
    text_type = 'text'
    if '<speak>' in tts_info['message']:
        text_type = 'ssml'
    try:
        response = polly_client.synthesize_speech(
            OutputFormat='json',
            Text=tts_info['message'],
            TextType=text_type,
            VoiceId=tts_info['voice'],
            SpeechMarkTypes=speech_mark_types
        )
    except botocore.exceptions.ClientError as e:
        if "(ValidationException)" in e.message:
            raise ClientError("Malformed Polly request")
        raise e

    write_data_to_cache(response["AudioStream"], "{}-{}".format(generate_key(tts_info), tts_info['speechMarks']), "{}.txt", from_cgp)
    return generate_presigned_url(tts_key)

def add_prosody_tags_to_message(message, character_info):
    message_content = message
    if "<speak>" in message:
        start_index = message.find("<speak>")
        end_index = message.find("</speak>")
        message_content = message[start_index + 7 : end_index]

    if "ssmlLanguage" in character_info:
        ssml_lang_tag = "lang=\"" + character_info.get("ssmlLanguage") + "\""
        message_content = "<lang xml:" + ssml_lang_tag + ">" + message_content + "</lang>"

    if "timbre" in character_info:
        message_content = "<amazon:effect vocal-tract-length=\"" + \
            str(character_info.get("timbre")) + "%\">" + \
            message_content + "</amazon:effect>"

    if "ssmlProsodyTags" in character_info:
        ssml_prosody_tags = character_info.get("ssmlProsodyTags")
        if len(ssml_prosody_tags) > 0:
            prosody_tag = ""
            for ssml_prosody_tag in ssml_prosody_tags:
                prosody_tag += " " + ssml_prosody_tag
            message_content = "<prosody" + prosody_tag + ">" + message_content + "</prosody>"
    
    message = "<speak>" + message_content + "</speak>"

    return message

def __get_speech_mark_types(speech_marks):
    speech_mark_types = []
    if 'S' in speech_marks:
        speech_mark_types.append('sentence')
        speech_mark_types.append('word')
    if 'V' in speech_marks:
        speech_mark_types.append('viseme')
    if 'T' in speech_marks:
        speech_mark_types.append('ssml')
    return speech_mark_types


def get_voice(tts_info, from_cgp = False):
    '''Returns the url of the sound file for the given string/voice'''
    if not (from_cgp or runtime_capabilities_enabled()):
        return ""

    if len(tts_info["voice"]) == 0:
        raise ClientError("Invalid request, Voice parameter was empty")

    tts_key = "{}.pcm".format(generate_key(tts_info))
    if key_exists(tts_key, TTSCACHE):
        return generate_presigned_url(tts_key)

    text_type = 'text'
    if '<speak>' in tts_info['message']:
        text_type = 'ssml'
    try:
        polly_client = boto3.client("polly")
        response = polly_client.synthesize_speech(
            OutputFormat='pcm',
            Text=tts_info["message"],
            TextType=text_type,
            VoiceId=tts_info["voice"]
        )
    except botocore.exceptions.ClientError as e:
        if "(ValidationException)" in e.message:
            raise ClientError("Malformed Polly request")
        raise e


    write_data_to_cache(response["AudioStream"], generate_key(tts_info), "{}.pcm", from_cgp)
    return generate_presigned_url(tts_key)


def enable_runtime_capabilities(enable):
    s3 = boto3.client('s3')
    if enable and key_exists(RUNTIME_CAPABILITIES_FLAG_KEY, TTSCACHE):
        s3.delete_object(Bucket=CloudCanvas.get_setting(TTSCACHE), Key=RUNTIME_CAPABILITIES_FLAG_KEY)
    elif not enable and not key_exists(RUNTIME_CAPABILITIES_FLAG_KEY, TTSCACHE):
        s3.put_object(Bucket=CloudCanvas.get_setting(TTSCACHE), Key=RUNTIME_CAPABILITIES_FLAG_KEY)


def runtime_capabilities_enabled():
    return not key_exists(RUNTIME_CAPABILITIES_FLAG_KEY, TTSCACHE)


def enable_cache_runtime_generated_files(enable):
    s3 = boto3.client('s3')
    if enable and not cache_runtime_generated_files():
        s3.delete_object(Bucket=CloudCanvas.get_setting(TTSCACHE), Key=RUNTIME_CACHING_FLAG_KEY)
    elif not enable and cache_runtime_generated_files():
        s3.put_object(Bucket=CloudCanvas.get_setting(TTSCACHE), Key=RUNTIME_CACHING_FLAG_KEY)


def cache_runtime_generated_files():
    return not key_exists(RUNTIME_CACHING_FLAG_KEY, TTSCACHE)


def key_exists(key, bucket_name):
    '''
    Returns True if the given key exists in the bucket
    '''
    bucket = get_bucket(bucket_name)
    objects = list(bucket.objects.filter(Prefix=key))
    return (len(objects) > 0) and (objects[0].key == key)


def generate_key(tts_info):
    return hashlib.md5("{}-{}".format(tts_info["voice"], tts_info["message"])).hexdigest()


def generate_presigned_url(key):
    '''
    Returns a presigned url for the given key in ttscache bucket
    '''
    s3_client = boto3.client('s3')
    if cache_runtime_generated_files():
        return s3_client.generate_presigned_url('get_object',
            Params = { "Bucket" : CloudCanvas.get_setting(TTSCACHE), "Key" : key })
    else:
        return s3_client.generate_presigned_url('get_object',
            Params = { "Bucket" : CloudCanvas.get_setting(TTSCACHE), "Key" : key },
            ExpiresIn = 300)


def write_data_to_cache(stream, key, file_name_template, from_cgp):
    '''
    Write data from stream to object with the given key in ttscache bucket
    '''
    data = stream.read()
    tts_bucket = get_bucket(TTSCACHE)
    tts_object = tts_bucket.Object(file_name_template.format(key))
    if not from_cgp and not cache_runtime_generated_files():
        tts_object.put(Body=data, Tagging="letexpire=true")
    else:
        tts_object.put(Body=data)

def get_bucket(bucket_name):
    global BUCKET
    if BUCKET[bucket_name] == None:
        BUCKET[bucket_name] = boto3.resource("s3").Bucket(CloudCanvas.get_setting(bucket_name))
    return BUCKET[bucket_name]

def save_custom_mappings(custom_mappings):
    get_bucket(TTSCACHE).put_object(Key='import_custom_mappings.json', Body=json.dumps(custom_mappings))

def get_custom_mappings():
    file_key = 'import_custom_mappings.json'
    custom_mappings = {}
    if key_exists(file_key, TTSCACHE):
        client = boto3.client('s3')
        response = client.get_object(Bucket=CloudCanvas.get_setting(TTSCACHE), Key=file_key)
        custom_mappings = json.loads(response["Body"].read())
    return custom_mappings
