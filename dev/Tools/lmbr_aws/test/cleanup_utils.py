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
# $Revision: #26 $

import boto3
import os
import json
import sys
from ConfigParser import RawConfigParser
from StringIO import StringIO

DEFAULT_REGION='us-east-1'


def get_user_settings_path():
    '''Reads {root}\bootstrap.cfg to determine the name of the game directory.'''

    root_directory_path = os.getcwd()
    while os.path.basename(root_directory_path) != 'dev':
        root_directory_path = os.path.dirname(root_directory_path)

    path = os.path.join(root_directory_path, "bootstrap.cfg")

    if not os.path.exists(path):
        print 'Warning: a bootstrap.cfg file was not found at {}, using "Game" as the project directory name.'.format(path)
        return 'Game'
    else:
        # If we add a section header and change the comment prefix, then
        # bootstrap.cfg looks like an ini file and we can use ConfigParser.
        ini_str = '[default]\n' + open(path, 'r').read()
        ini_str = ini_str.replace('\n--', '\n#')
        ini_fp = StringIO(ini_str)
        config = RawConfigParser()
        config.readfp(ini_fp)

        game_directory_name = config.get('default', 'sys_game_folder')

        platform_mapping = {"win32": "pc", "darwin": "osx_gl"}
        user_settings_path = os.path.join(root_directory_path, 'Cache', game_directory_name, platform_mapping[sys.platform], 'User\AWS\user-settings.json')

        if not os.path.exists(user_settings_path):
            raise HandledError('{} does not exist.'.format(user_settings_path))

        return user_settings_path


def get_default_profile(args):
    user_settings_path = get_user_settings_path()
    profile = 'default'

    if os.path.isfile(user_settings_path):
        with open(user_settings_path, 'r') as file:
            settings = json.load(file)
            profile = settings.get('DefaultProfile', args.profile)

    return profile


def add_common_arguments(parser):
    parser.add_argument('--profile', default='default', help='The AWS profile to use. Defaults to the default AWS profile.')
    parser.add_argument('--aws-access-key', required=False, help='The AWS access key to use.')
    parser.add_argument('--aws-secret-key', required=False, help='The AWS secret key to use.')
    parser.add_argument('--region', default=DEFAULT_REGION, help='The AWS region to use. Defaults to {}.'.format(DEFAULT_REGION))


def get_session(args):
    if args.aws_access_key and args.aws_secret_key:
        session = boto3.Session(aws_access_key_id=args.aws_access_key, aws_secret_access_key=args.aws_secret_key, region_name=args.region)
    elif os.environ.get('NO_TEST_PROFILE', None):
        session = boto3.Session(region_name=args.region)
    else:
        args.profile = get_default_profile(args)
        session = boto3.Session(profile_name=args.profile, region_name=args.region)

    return session


def paginate(fn, params, paginator_key='Marker', next_paginator_key='NextMarker'):
    """
    A generic paginator that should work with any paginated AWS function, including those that do not have a built-in
    paginator supplied for them.

    :param fn: A client function to call, e.g. boto3.client('s3').list_objects
    :param params: A dictionary of parameters to pass into the function, e.g. {'Bucket': "foo"}
    :param paginator_key: The key used as the marker for fetching results, e.g. 'Marker'
    :param next_paginator_key: The key returned in the response to fetch the next page, e.g. 'NextMarker'
    :return: An iterator to the results of the function call.
    """
    while True:
        response = fn(**params)
        yield response

        next_key = response.get(next_paginator_key, None)
        if next_key and len(next_key):
            params[paginator_key] = next_key
        else:
            break


def nt_paginate(fn, params):
    return paginate(fn, params, 'NextToken', 'NextToken')
