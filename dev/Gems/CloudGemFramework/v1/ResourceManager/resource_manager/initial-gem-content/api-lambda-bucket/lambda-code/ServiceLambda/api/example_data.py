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
import errors
import bucket_data
from six import string_types


@service.api
def list(request, start=None, count=None):
    keys = bucket_data.list(start, count)
    return {
        'Keys': [{'Key': key} for key in keys]
    }


@service.api
def create(request, data):
    __validate_data(data)
    key = bucket_data.create(data)
    return {
        'Key': key
    }


@service.api
def read(request, key):
    data = bucket_data.read(key)
    if data is None:
        raise errors.HandledError('No data with key {} was found.'.format(key))
    return data


@service.api
def update(request, key, data):
    __validate_data(data)
    if not bucket_data.update(key, data):
        raise errors.HandledError('No data with key {} was found.'.format(key))


@service.api
def delete(request, key):
    if not bucket_data.delete(key):
        raise errors.HandledError('No data with key {} was found.'.format(key))


def __validate_data(data):
    if not isinstance(data.get('ExamplePropertyA', None), string_types):
        raise errors.HandledError('Property ExamplePropertyA in provided data is missing or is not a string.')

    if not isinstance(data.get('ExamplePropertyB', None), int):
        raise errors.HandledError('Property ExamplePropertyB in provided data is missing or is not an integer.')
