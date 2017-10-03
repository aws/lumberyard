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
from boto3.dynamodb.conditions import Key
import CloudCanvas
import errors

def is_non_negative_or_none(num):
    if is_none(num):
        return True

    return True if is_non_negative(num) else False

def is_non_negative(num):
    return True if num is not None and num >= 0 else False

def is_positive(num):
    return True if num is not None and num > 0 else False

def is_negative(num):
    return True if num is not None and num < 0 else False

def is_none(val):
    return True if val is None else False

def is_not_none(val):
    return True if val is not None else False

def is_not_empty(collecton):
    return True if collecton is not None and len(collecton) > 0 else False

def is_empty(collecton):
    return True if collecton is None or len(collecton) == 0 else False

def is_not_blank_str(str):
    return True if str is not None and len(str) > 0 and str.isspace() == False else False

def is_num_str(str):
    try:
        int(str)
    except ValueError:
        return False
    return True

def validate_param(param, param_name, validation_func):
    if validation_func(param) == False:
        raise errors.ClientError('{} is invalid: [{}]'.format(param_name, param))
