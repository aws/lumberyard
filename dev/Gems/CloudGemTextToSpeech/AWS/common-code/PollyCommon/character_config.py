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
import json
import os

from errors import ClientError
from botocore.client import Config

CHAR_BUCKET = None

def __get_bucket():
    global CHAR_BUCKET
    if CHAR_BUCKET == None:
        CHAR_BUCKET = boto3.resource('s3', config=Config(signature_version='s3v4')).Bucket(CloudCanvas.get_setting("characterdefs"))
    return CHAR_BUCKET

def get_all_characters():
    bucket = __get_bucket()
    return [os.path.splitext(obj.key)[0] for obj in bucket.objects.all()]

def get_character_info(character):
    if not character in get_all_characters():
        raise ClientError("Could not find {} in character list".format(character))
    client = boto3.client('s3')
    response = client.get_object(Bucket=CloudCanvas.get_setting("characterdefs"), Key="{}.json".format(character))
    body = json.loads(response["Body"].read())
    return body

def delete_character(character):
    if not character in get_all_characters():
        raise ClientError("Could not find {} in character list".format(character))
    client = boto3.client('s3')
    response = client.delete_object(Bucket=CloudCanvas.get_setting("characterdefs"), Key="{}.json".format(character))

def get_all_character_info():
    all_characters = get_all_characters()
    info = []
    for character in all_characters:
        info.append(get_character_info(character))
    return info

def add_character(character_def):
    if character_def["name"] in get_all_characters():
        raise ClientError("Character {} already exists!".format(character_def["name"]))
    __get_bucket().put_object(Key="{}.json".format(character_def["name"]), Body=json.dumps(character_def))
    return character_def