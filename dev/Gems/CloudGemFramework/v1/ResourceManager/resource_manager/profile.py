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

import os
from resource_manager_common import constant

from errors import HandledError
from util import Args

import boto3
from botocore.exceptions import ClientError

def list(context, args):

    list = []

    credentials = context.aws.load_credentials()

    for section_name in credentials.sections():
        if args.profile is None or section_name == args.profile:

            try:
                # Not using context.aws.client because it always returns clients 
                # configured using the --profile argument. In this case we want to
                # use each profile's credentials.
                sts = boto3.client('sts', 
                        aws_access_key_id = credentials.get(section_name, constant.ACCESS_KEY_OPTION), 
                        aws_secret_access_key = credentials.get(section_name, constant.SECRET_KEY_OPTION))
                res = sts.get_caller_identity()
            except ClientError as e:
                if e.response['Error']['Code'] not in ['InvalidClientTokenId', 'SignatureDoesNotMatch']:
                    raise e
                res = {}

            arn = res.get('Arn', 'arn:aws:iam::554229317296:user/(unknown)')
            user_name = arn[arn.rfind('/')+1:]

            list.append(
                {
                    'Name': section_name,
                    'AccessKey': credentials.get(section_name, constant.ACCESS_KEY_OPTION),
                    'SecretKey': credentials.get(section_name, constant.SECRET_KEY_OPTION),
                    'Account': res.get('Account', '(unknown)'),
                    'UserName': user_name,
                    'Default': section_name == context.config.user_default_profile
                })

    context.view.profile_list(list, context.aws.get_credentials_file_path())

def add(context, args):

    credentials = context.aws.load_credentials()

    if credentials.has_section(args.profile):
        raise HandledError('The AWS profile {} already exists.'.format(args.profile))

    # do not accept empty strings
    if not args.profile:
        raise HandledError('Cannot create a profile with an empty name')

    credentials.add_section(args.profile)
    credentials.set(args.profile, constant.SECRET_KEY_OPTION, args.aws_secret_key)
    credentials.set(args.profile, constant.ACCESS_KEY_OPTION, args.aws_access_key)

    context.aws.save_credentials(credentials)

    context.view.added_profile(args.profile)

    if(args.make_default):
        nested_args = Args()
        nested_args.set = args.profile
        default(context, nested_args)


def update(context, args):

    credentials = context.aws.load_credentials()

    if args.old_name is None and args.new_name is None:
        if not credentials.has_section(args.profile):
            raise HandledError('The AWS profile {} does not exist.'.format(args.profile))
    else:
        if args.old_name != args.new_name:
            __rename(credentials, args.old_name, args.new_name)
        args.profile = args.new_name

    if args.aws_secret_key is not None:
        credentials.set(args.profile, constant.SECRET_KEY_OPTION, args.aws_secret_key)

    if args.aws_access_key is not None:
        credentials.set(args.profile, constant.ACCESS_KEY_OPTION, args.aws_access_key)

    context.aws.save_credentials(credentials)

    if context.config.user_default_profile == args.old_name and args.old_name != args.new_name:
        context.config.set_user_default_profile(args.new_name)

    context.view.updated_profile(args.profile)


def remove(context, args):

    credentials = context.aws.load_credentials()

    if not credentials.has_section(args.profile):
        raise HandledError('The AWS profile {} does not exist.'.format(args.profile))

    credentials.remove_section(args.profile)

    context.aws.save_credentials(credentials)

    if context.config.user_default_profile == args.profile:
        context.config.clear_user_default_profile()
        context.view.removed_default_profile(args.profile)
    else:
        context.view.removed_profile(args.profile)


def rename(context, args):

    credentials = context.aws.load_credentials()

    __rename(credentials, args.old_name, args.new_name)

    context.aws.save_credentials(credentials)

    if context.config.user_default_profile == args.old_name:
        context.config.set_user_default_profile(args.new_name)

    context.view.renamed_profile(args.old_name, args.new_name)
   
def __rename(credentials, old_name, new_name):

    if not credentials.has_section(old_name):
        raise HandledError('The AWS profile {} does not exist.'.format(old_name))

    if credentials.has_section(new_name):
        raise HandledError('An AWS profile {} already exists.'.format(new_name))

    if not new_name:
        raise HandledError('Cannot rename a profile an empty name')

    access_key = credentials.get(old_name, constant.ACCESS_KEY_OPTION)
    secret_key = credentials.get(old_name, constant.SECRET_KEY_OPTION)

    credentials.add_section(new_name)
    credentials.set(new_name, constant.SECRET_KEY_OPTION, secret_key)
    credentials.set(new_name, constant.ACCESS_KEY_OPTION, access_key)

    credentials.remove_section(old_name)

def default(context, args):

    if args.set is not None:
        credentials = context.aws.load_credentials()
        if not credentials.has_section(args.set):
            raise HandledError('The AWS profile "{}" does not exist.'.format(args.set))
        context.config.set_user_default_profile(args.set)
    elif args.clear:
        context.config.clear_user_default_profile()

    context.view.default_profile(context.config.user_default_profile)
