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

import CloudCanvas
import boto3

IDENTITY_MAP = None


def init_resources():
    global IDENTITY_MAP
    if not IDENTITY_MAP:
        try:
            IDENTITY_MAP = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("UserIdentityMap"))
        except Exception as e:
            raise Exception("Error retrieving identity table")


def validate_user(user, cognito_id):
    if not cognito_id:
        return False

    init_resources()
    response = IDENTITY_MAP.get_item(Key = {"user": user})
    item = response.get("Item", {})
    if not item:
        item["user"] = user
        item["cognito_id"] = cognito_id
        IDENTITY_MAP.put_item(Item=item)
        return True

    return item.get("cognito_id", "") == cognito_id
