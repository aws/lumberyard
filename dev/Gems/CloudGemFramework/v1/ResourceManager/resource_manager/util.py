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
# $Revision: #1 $

import copy
import os
import json
import re

from errors import HandledError
from botocore.exceptions import ClientError
from resource_manager_common import resource_type_info

def default(value, default):
    '''Returns a default value when a given value is None, otherwise returns the given value.'''
    if value is None:
        return default
    else:
        return value

class Args(object):

    '''An object that returns None for any undefined properties.'''

    def __init__(self, **kwargs):
        if kwargs:
            for k, v in kwargs.iteritems():
                self.__dict__[k] = v

    def __str__(self):
        return str(self.__dict__)

    def __getattr__(self, name):
        if name in self.__dict__:
            return self.__dict__[name]
        else:
            return None

def replace_string_in_dict(dict, old, new):
    result = {}
    for k, v in dict.iteritems():
        result[k.replace(old, new)] = replace_string_in_value(v, old, new)
    return result

def replace_string_in_value(value, old, new):
    if isinstance(value, basestring):
        return value.replace(old, new)
    if isinstance(value, list):
        return replace_string_in_list(value, old, new)
    if isinstance(value, dict):
        return replace_string_in_dict(value, old, new)
    return value

def replace_string_in_list(list, old, new):
    result = []
    for entry in list:
        result.append(replace_string_in_value(entry, old, new))
    return result

def load_template(context, template_path):
    '''Reads template file content and returns it as a string.'''
    try:
        return open(template_path, 'r').read()
    except IOError as e:
        raise HandledError('Could not read template from {0}.'.format(template_path))

def validate_stack_name_length(check_name):

    name_length = len(check_name)

    if name_length > 128:
        raise HandledError('Name is {} characters, limit 128: {}'.format(name_length, check_name))

def validate_stack_name_format(check_name):

    if not re.match('^[a-z][a-z0-9\-]*$', check_name, re.I):
        raise HandledError('Name can only consist of letters, numbers and hyphens and must start with a letter: {}'.format(check_name))

def validate_stack_name_no_reserved_words(check_name):
    if check_name in resource_type_info.LAMBDA_TAGS:
        raise HandledError('Name must not be any of %s' % sorted(list(resource_type_info.LAMBDA_TAGS)))

def validate_stack_name(check_name):

    if check_name == None:
        raise HandledError('No valid name provided')

    validate_stack_name_length(check_name)

    validate_stack_name_format(check_name)

    validate_stack_name_no_reserved_words(check_name)

def validate_writable_list(context, write_check_list):
    '''Prompts the user to make specified files writable.

    Arguments:

        write_check_list - The list of file paths that should be writable.

    Returns:

        True once all files are writable, or False if the user chooses to
        not make one or more files writable.

    Raises:

        The cli version raise HandledException instead of returning False.

    '''
    
    while True:

        fail_list = []
        for this_file in write_check_list:
            if not context.config.is_writable(this_file):
                fail_list.append(this_file)

        if len(fail_list) == 0:
            return True

        if not context.view.confirm_writable_try_again(fail_list):
            return False

def get_cloud_canvas_metadata(definition, metadata_name):

    metadata = definition.get('Metadata', None)
    if metadata is None: return None

    cloud_canvas_metadata = metadata.get('CloudCanvas', None)
    if cloud_canvas_metadata is None: return None

    return cloud_canvas_metadata.get(metadata_name, None)

# Stack ARN format: arn:aws:cloudformation:{region}:{account}:stack/{name}/{guid}

def get_stack_name_from_arn(arn):
    if arn is None: return None
    return arn.split('/')[1]

def get_region_from_arn(arn):
    if arn is None: return None
    return arn.split(':')[3]

def get_account_id_from_arn(arn):
    if arn is None: return None
    return arn.split(':')[4]

ID_DATA_MARKER = '::'

def get_data_from_custom_physical_resource_id(physical_resource_id):
    if physical_resource_id:
        i_data_marker = physical_resource_id.find(ID_DATA_MARKER)
        if i_data_marker == -1:
            id_data = {}
        else:
            try:
                id_data = json.loads(physical_resource_id[i_data_marker+len(ID_DATA_MARKER):])
            except Exception as e:
                raise HandledError('Could not parse JSON data from physical resource id {}. {}'.format(physical_resource_id, e.message))
    else:
        id_data = {}
    return id_data

def trim_at(s, c):
    i = s.find(c)
    if i == -1:
        return s
    else:
        return s[:i]

def dict_get_or_add(dict, name, default):
    if name not in dict:
        dict[name] = default
    return dict[name]

def save_json(path, data):
    '''Writes a dictionary to a file on disk using the JSON format.'''
    try:
        dir = os.path.dirname(path)
        if not os.path.exists(dir):
            os.makedirs(dir)
        json_content = json.dumps(data, indent=4, sort_keys=True)
        with open(path, 'w') as file:
            file.write(json_content)
        return True
    except Exception as e:
        raise HandledError('Could not save {}.'.format(path), e)
        return False

def load_json(path, default = None, optional = True):
    '''Reads JSON format data from a file on disk and returns it as dictionary.'''
    try:
        if os.path.isfile(path):
            with open(path, 'r') as file:
                data = file.read()
            if len(data):
                return json.loads(data)
            elif not optional:
                raise HandledError('Could not load {}. The file is empty.'.format(path))
            else:
                return copy.deepcopy(default)
        elif not optional:
            raise HandledError('Could not load {}. The file does not exist.'.format(path))
        else:
            return copy.deepcopy(default)
    except Exception as e:
        raise HandledError('Could not load {}.'.format(path), e)

def json_parse(str, default):
    '''Reads JSON format data from a file on disk and returns it as dictionary.'''
    try:
        if str:
            json.load(str);
        else:
            return default
    except Exception as e:
        raise HandledError('Could not parse {}.'.format(str), e)


def delete_bucket_contents(context, stack_name, logical_bucket_id, physical_bucket_id):

    s3 = context.aws.client('s3')

    try:
        list_res = s3.list_object_versions(Bucket=physical_bucket_id, MaxKeys=500)
    except ClientError as e:
        if e.response['Error']['Code'] == 'NoSuchBucket':
            return

    total = 0

    while True:

        delete_list = []

        for version in list_res.get('Versions', []):
            delete_list.append(
                {
                    'Key': version['Key'],
                    'VersionId': version['VersionId']
                })

        for marker in list_res.get('DeleteMarkers', []):
            delete_list.append(
                {
                    'Key': marker['Key'],
                    'VersionId': marker['VersionId']
                })

        if delete_list:

            count = len(delete_list)
            total += count

            context.view.deleting_bucket_contents(stack_name, logical_bucket_id, count, total)

            s3.delete_objects(Bucket=physical_bucket_id, Delete={ 'Objects': delete_list, 'Quiet': True })

        if 'NextKeyMarker' not in list_res or 'NextVersionIdMarker' not in list_res:
            break

        list_res = s3.list_object_versions(Bucket=physical_bucket_id, MaxKeys=500, KeyMarker=list_res['NextKeyMarker'], VersionIdMarker=list_res['NextVersionIdMarker'])




