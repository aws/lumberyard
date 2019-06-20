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

import tts
import urllib
import boto3
import zipfile
import json
import CloudCanvas
import character_config
import uuid
import csv

from errors import ClientError
from botocore.client import Config

import text_to_speech_s3

PACKAGEDVOICELINES = 'packagedvoicelines'
ID_PACKAGES_FOLDER = "idPackages/"

def create_zip_file_async(request_info):
    download_info = {}
    download_info['tts_info_list'] = request_info['entries']
    download_info['name'] = request_info['name']
    download_info['UUID'] = str(uuid.uuid4())
    json_data = json.dumps(download_info)

    client = boto3.client('lambda')
    client.invoke_async(
        FunctionName=CloudCanvas.get_setting('PackageVoiceLines'),
        InvokeArgs=json_data
    )

    return download_info['name'] + download_info['UUID']


def get_id_package_name(name, UUID):
    return "{}{}{}".format(ID_PACKAGES_FOLDER, name, UUID)


def check_url(name, in_lib = True):
    zip_file_key = name + '.zip'
    if not in_lib:
        zip_file_key = ID_PACKAGES_FOLDER + name + ".zip"

    url = 'pending'
    if tts.key_exists(zip_file_key, PACKAGEDVOICELINES):
        url = __generate_presigned_url(zip_file_key)

    return url

def get_generated_packages():
    s3_client = text_to_speech_s3.get_s3_client()
    response = s3_client.list_objects(Bucket = CloudCanvas.get_setting(PACKAGEDVOICELINES))

    generated_packages = []
    for s3_object in response.get('Contents', []):
        if not ID_PACKAGES_FOLDER in s3_object['Key']:
            generated_packages.append({'name': s3_object['Key'], 'lastModified': s3_object['LastModified'].strftime("%Y-%m-%d %H:%M:%S"), 'size': s3_object['Size']})
    return generated_packages

def create_zip_file(tts_info_list, name, UUID):
    if len(tts_info_list) == 0:
        return ''

    zip_file_name = '/tmp/export.zip'
    zip_file_key = name + '.zip'
    unique_zip_key = get_id_package_name(name, UUID) + ".zip"
    zf = zipfile.ZipFile(zip_file_name, 'w')
    character_mapping = {}
    speech_line_definitions = []
    
    character_custom_mapping = 'Character'
    line_custom_mapping = 'Line'
    custom_mappings = tts.get_custom_mappings()
    for key, value in custom_mappings.items():
        if value == 'character':
            character_custom_mapping = key
        elif value == 'line':
            line_custom_mapping = key
    speech_lines_header = [character_custom_mapping, line_custom_mapping, 'MD5']

    for tts_info in tts_info_list:
        character_info = character_config.get_character_info(tts_info['character'])
        tts_info['voice'] = character_info['voice']
        tts_info['message'] = tts.add_prosody_tags_to_message(tts_info['line'], character_info)
        tts_info['speechMarks'] = character_info['speechMarks']
        character_mapping[tts_info['character']] = {
            'voice': character_info['voice'], 'speechMarks': character_info['speechMarks']
        }
        if character_info.get('ssmlProsodyTags', []):
            character_mapping[tts_info['character']]['ssmlTags'] = character_info['ssmlProsodyTags']
        if character_info.get('ssmlLanguage',''):
            character_mapping[tts_info['character']]['ssmlLanguage'] = character_info['ssmlLanguage']
        if character_info.get('timbre',''):
            character_mapping[tts_info['character']]['timbre'] = character_info['timbre']

        __add_speech_line_definition(tts_info, speech_line_definitions, speech_lines_header)
        __update_spoken_line_file(tts_info, zip_file_name)
        __update_speech_marks_file(tts_info, zip_file_name)
   
    __create_character_mappings_file(zip_file_name, character_mapping)
    __create_speech_definitions_file(zip_file_name, speech_line_definitions, speech_lines_header)
    __upload_zip_file(zip_file_name, zip_file_key)
    __upload_zip_file(zip_file_name, unique_zip_key)

    message = 'The zip file {} is generated successfully.'.format(zip_file_key)
    print(message)

def delete_zip_file(key):
    client = text_to_speech_s3.get_s3_client()
    try:
        client.delete_object(Bucket=CloudCanvas.get_setting(PACKAGEDVOICELINES), Key = key)
    except ClientError as e:
        return "ERROR: " + e.response['Error']['Message']
    return 'success'

def __update_spoken_line_file(tts_info, zip_file_name):
    spoken_line_url = tts.get_voice(tts_info, True)
    spoken_line_key = '{}.pcm'.format(tts.generate_key(tts_info))

    __add_to_zip(spoken_line_key, spoken_line_url, zip_file_name)

def __update_speech_marks_file(tts_info, zip_file_name):
    if not tts_info['speechMarks']:
        return

    speech_marks_url = tts.get_speech_marks(tts_info, True)
    speech_marks_key = '{}-{}.txt'.format(tts.generate_key(tts_info), tts_info['speechMarks'])

    __add_to_zip(speech_marks_key, speech_marks_url, zip_file_name)

def __add_to_zip(key, url, zip_file_name):
    zf = zipfile.ZipFile(zip_file_name, 'a')
    try:
        if not key.rstrip('/') in zf.namelist():
            urllib.urlretrieve(url, '/tmp/' + key)
            zf.write('/tmp/' + key, key)
    except:
        error_message = 'Could not add {} to the zip file {}.'.format(key, zip_file_name)
        raise ClientError(error_message)
    finally:
        zf.close()

def __create_character_mappings_file(zip_file_name, character_mapping):
    with open('/tmp/character_mapping.json', 'w') as file:
        json.dump(character_mapping, file)

    zf = zipfile.ZipFile(zip_file_name, 'a')
    try:
        zf.write('/tmp/character_mapping.json', '/character_mapping.json')
    except:
        error_message = 'Could not add character mappings to the zip file {}.'.format(zip_file_name)
        raise ClientError(error_message)
    finally:
        zf.close()

def __add_speech_line_definition(tts_info, speech_line_definitions, speech_lines_header):
    speech_line_definition = {}
    speech_line_definition[speech_lines_header[0]] = tts_info['character']
    speech_line_definition[speech_lines_header[1]] = tts_info['line']

    for tag in tts_info['tags']:
        key = 'Tags'
        value = tag
        if ':' in tag:
            key = tag.split(':')[0]
            value = tag.split(':')[1]

        if key not in speech_lines_header:
            speech_lines_header.append(key)

        if (key in speech_line_definition.keys()):
            speech_line_definition[key] = speech_line_definition[key] + ',' + value
        else:
            speech_line_definition[key] = value;

    speech_line_definition['MD5'] = tts.generate_key(tts_info)
    speech_line_definitions.append(speech_line_definition)

def __create_speech_definitions_file(zip_file_name, speech_line_definitions, speech_lines_header):
    with open('/tmp/speech_line_definitions.csv', 'w') as file:
        writer = csv.DictWriter(file, fieldnames = speech_lines_header)
        writer.writeheader()
        for speech_line in speech_line_definitions:
            writer.writerow(speech_line)

    zf = zipfile.ZipFile(zip_file_name, 'a')
    try:
        zf.write('/tmp/speech_line_definitions.csv', '/speech_line_definitions.csv')
    except:
        error_message = 'Could not add speech line definitions to the zip file {}.'.format(zip_file_name)
        raise ClientError(error_message)
    finally:
        zf.close()

def __upload_zip_file(file_name, key):
    s3 = boto3.resource('s3', config=Config(signature_version='s3v4'))
    try:
        s3.meta.client.upload_file(file_name, CloudCanvas.get_setting(PACKAGEDVOICELINES), key)
    except:
        error_message = 'Could not upload the zip file {}.'.format(key)
        raise ClientError(error_message)

def __generate_presigned_url(key):
    s3_client = text_to_speech_s3.get_s3_client()
    try:
        presigned_url = s3_client.generate_presigned_url('get_object', Params =
            {
                'Bucket' : CloudCanvas.get_setting(PACKAGEDVOICELINES),
                'Key' : key
            })
    except:
        error_message = 'Could not generate the presigned URL for {}.'.format(key);
        raise ClientError(error_message)
    return presigned_url

    tts.get_bucket(PACKAGEDVOICELINES).put_object(Key=file_key , Body=json.dumps(export_info))
