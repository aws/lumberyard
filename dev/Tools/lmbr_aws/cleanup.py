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
# $Revision: #2 $

from __future__ import print_function
import argparse
import boto3
import os
import json
import sys

import cleanup_utils.cleanup_cf_utils
import cleanup_utils.cleanup_s3_utils
import cleanup_utils.cleanup_iam_utils
import cleanup_utils.cleanup_cognito_identity_utils
import cleanup_utils.cleanup_cognito_idp_utils
import cleanup_utils.cleanup_logs_utils
import cleanup_utils.cleanup_apigateway_utils
import cleanup_utils.cleanup_dynamodb_utils
import cleanup_utils.cleanup_lambda_utils
import cleanup_utils.cleanup_sqs_utils
import cleanup_utils.cleanup_glue_utils
import cleanup_utils.cleanup_iot_utils
import cleanup_utils.cleanup_swf_utils

from botocore.client import Config

# Python 2.7/3.7 Compatibility
from six.moves import configparser
from six.moves import input
from six import StringIO

DEFAULT_REGION = 'us-east-1'

DEFAULT_PREFIXES = ['cctest']

DEFAULT_INTERVAL = 10  # seconds
DEFAULT_ATTEMPTS = 5  # seconds


def global_resource(cleanup_function):
    def wrapper(_cleaner):
        if _cleaner.delete_global_resources:
            cleanup_function(_cleaner)
        else:
            print("Skipping deletion of global {} (use --delete-global-resources or exclude --region to delete)".
                  format(cleanup_function.__name__[len("_delete_"):]))

    return wrapper


def _make_client_in_region(session, services, service_name, **kwargs):
    client = None
    if service_name in services:
        regions = boto3.session.Session().get_available_regions(service_name)
        if session.region_name in regions:
            client = session.client(service_name, **kwargs)
        else:
            print("[WARNING] AWS service {} is not available in {}. Skipping deletion of resources from service".format(
                service_name, session.region_name))
    else:
        print("[ERROR] Unknown AWS service requested {}. Skipping deletion of resources from service".format(service_name))
    return client


class Cleaner:

    def __init__(self, session, delete_global_resources, prefixes, exceptions):
        services = session.get_available_services()
        self.delete_global_resources = delete_global_resources

        # Services assumed to be either global or in all regions
        self.s3 = session.client('s3', config=Config(signature_version='s3v4'))
        self.iam = session.client('iam')
        self.dynamodb = session.client('dynamodb')

        # Services that may not be in all regions
        self.cf = _make_client_in_region(session, services, 'cloudformation')
        self.cognito_identity = _make_client_in_region(session, services, 'cognito-identity')
        self.cognito_idp = _make_client_in_region(session, services, 'cognito-idp')
        self.logs = _make_client_in_region(session, services, 'logs')
        self.apigateway = _make_client_in_region(session, services, 'apigateway')

        self.lambda_client = _make_client_in_region(session, services, 'lambda')
        self.sqs = _make_client_in_region(session, services, 'sqs')
        self.glue = _make_client_in_region(session, services, 'glue')
        self.iot_client = _make_client_in_region(session, services, 'iot')
        self.swf = _make_client_in_region(session, services, 'swf')
        self.wait_interval = DEFAULT_INTERVAL
        self.wait_attempts = DEFAULT_ATTEMPTS

        self._prefixes = prefixes
        self._exceptions = exceptions
        self._failed_resources = {}
        self._previous_failures = {}

    def _run_all_cleanup(self):
        if self.cf:
            cleanup_utils.cleanup_cf_utils.delete_cf_stacks(self)
        if self.cognito_identity:
            cleanup_utils.cleanup_cognito_identity_utils.delete_identity_pools(self)
        if self.cognito_idp:
            cleanup_utils.cleanup_cognito_idp_utils.delete_user_pools(self)
        if self.logs:
            cleanup_utils.cleanup_logs_utils.delete_log_groups(self)
        if self.apigateway:
            cleanup_utils.cleanup_apigateway_utils.delete_api_gateway(self)
        if self.lambda_client:
            cleanup_utils.cleanup_lambda_utils.delete_lambdas(self)
        if self.sqs:
            cleanup_utils.cleanup_sqs_utils.delete_sqs_queues(self)
        if self.glue:
            cleanup_utils.cleanup_glue_utils.delete_glue_crawlers(self)
            cleanup_utils.cleanup_glue_utils.delete_glue_databases(self)
        if self.swf:
            cleanup_utils.cleanup_swf_utils.delete_swf_resources(self)

        # 'Global' services
        cleanup_utils.cleanup_dynamodb_utils.delete_dynamodb_tables(self)
        cleanup_utils.cleanup_s3_utils.delete_s3_buckets(self)

        # IAM users/roles/policies need to be deleted last
        cleanup_utils.cleanup_iam_utils.delete_iam_users(self)
        cleanup_utils.cleanup_iam_utils.delete_iam_roles(self)
        cleanup_utils.cleanup_iam_utils.delete_iam_policies(self)
        cleanup_utils.cleanup_iot_utils.delete_iot_policies(self)

    def cleanup(self):
        self._run_all_cleanup()

        # retry if we have failures
        if len(self._failed_resources) > 0:
            print("\n\nFirst cleanup finished, but we failed to delete all resources. Retrying cleanup.")
            self._previous_failures = self._failed_resources.copy()
            self._failed_resources = {}
            self._run_all_cleanup()

            # keep retrying if we keep finding things to delete
            while len(self._failed_resources) > 0 and self._failed_resources != self._previous_failures:
                print("\n\nThere are still more failed resources to delete. Retrying cleanup.")
                self._previous_failures = self._failed_resources.copy()
                self._failed_resources = {}
                self._run_all_cleanup()

            if len(self._failed_resources) > 0:
                for service in self._failed_resources:
                    print("[ERROR] Failed to delete {}:{}".format(service, self._failed_resources[service]))
                raise RuntimeError("Failed to clean up all resources")

    def has_prefix(self, name):
        name = name.lower()

        for exception in self._exceptions:
            if name.startswith(exception):
                return False

        for prefix in self._prefixes:
            if name.startswith(prefix):
                return True

        return False

    def describe_prefixes(self):
        return str(self._prefixes) + ' but not ' + str(self._exceptions)

    def add_to_failed_resources(self, service, resource):
        """ Add to map of service --> [failed resources] """
        if service in self._failed_resources:
            self._failed_resources[service].append(resource)
        else:
            self._failed_resources[service] = [resource]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--prefix', dest='prefixes', nargs='+', default=DEFAULT_PREFIXES,
                        help='Any stacks and buckets with names that start with this value will be deleted.')
    parser.add_argument('--except', dest='exceptions', nargs='+', default=[],
                        help='Do not delete anything starting with these prefixes (useful for cleaning up old results '
                             'while a test is in progress).')
    parser.add_argument('--profile', default='default',
                        help='The AWS profile to use. Defaults to the default AWS profile. AWS access and secret key '
                             'take priority over profile if set.')
    parser.add_argument('--aws-access-key', required=False, help='The AWS access key to use. Requires an AWS secret '
                                                                 'key.')
    parser.add_argument('--aws-secret-key', required=False, help='The AWS secret key to use. Requires an AWS access '
                                                                 'key')
    parser.add_argument('--region', default=None, help='The AWS region to use. Defaults to {} if not specified.'.
                        format(DEFAULT_REGION))
    parser.add_argument('--delete-global-resources', action='store_true', default=False,
                        help='If --region specified, use to delete global resources such as IAM roles and S3 buckets. '
                             'Ignored if --region is unspecified.')
    parser.add_argument('--confirm-deletion', action='store_true', default=False,
                        help='Confirms that you know this command will delete AWS resources.')

    args, unknown_args = parser.parse_known_args()

    if not __confirmation(args):
        return

    prefixes = []
    for prefix in args.prefixes:
        prefixes.append(prefix.lower())

    exceptions = []
    for exception in args.exceptions:
        exceptions.append(exception.lower())

    use_region = args.region if args.region else DEFAULT_REGION

    if args.aws_access_key and args.aws_secret_key:
        session = boto3.Session(aws_access_key_id=args.aws_access_key, aws_secret_access_key=args.aws_secret_key,
                                region_name=use_region)
    elif args.profile != 'default':
        session = boto3.Session(profile_name=args.profile, region_name=use_region)
    elif os.environ.get('NO_TEST_PROFILE', None):
        session = boto3.Session(region_name=use_region)
    else:
        args.profile = __get_default_profile(args)
        session = boto3.Session(profile_name=args.profile, region_name=use_region)

    delete_global_resources = True
    if args.region and not args.delete_global_resources:
        print("WARNING: Specifying a --region skips deletion of global resources like IAM roles and S3 buckets. Add "
              "--delete-global-resources or do not specify a region to include these resources in deletion.")
        delete_global_resources = False
    elif args.delete_global_resources and not args.region:
        print("WARNING: --delete-global-resources is ignored without specifying a --region. (Global resources are "
              "always included for deletion if --region is left unspecified.)")
    elif not args.region:
        print("WARNING: No --region was specified.  The cleanup will default to the region '{}' and delete any matching"
              " global resources.".format(DEFAULT_REGION))

    cleaner = Cleaner(session, delete_global_resources, prefixes, exceptions)
    cleaner.cleanup()


def __confirmation(args):
    """
    Prompts the user to confirm usage of the cleanup tool. Returns a boolean depending on their answer. Also takes in
    the confirm-deletion arg to bypass the prompt.
    :return: True if the user enters Yes or the confirm-deletion arg exists.
    """
    if args.confirm_deletion:
        return True

    confirmation_prompt_string = 'WARNING: Do not use the cleanup tool if you have a project stack name that begins ' \
                                 'with an IAM user name that you do not want to delete. Doing so can result in the ' \
                                 'deletion of the IAM user, its roles, and its profiles. When you delete an AWS ' \
                                 'resource, you permanently delete any objects that are stored in that resource. ' \
                                 'For example, if you delete an S3 bucket, all objects inside the bucket are also ' \
                                 'deleted. Do you want to continue? (Yes/No)'
    yes_answers = ['y', 'yes']

    prompt_answer = input(confirmation_prompt_string)

    if prompt_answer.lower() not in yes_answers:
        return False
    return True


def __get_user_settings_path():
    """
    Reads {root}\\bootstrap.cfg to determine the name of the game directory
    :return: The path to the user-settings.json file
    """

    root_directory_path = os.getcwd()
    while os.path.basename(root_directory_path) != 'dev':
        root_directory_path = os.path.dirname(root_directory_path)

    path = os.path.join(root_directory_path, "bootstrap.cfg")

    if not os.path.exists(path):
        print('Warning: a bootstrap.cfg file was not found at {}, using "Game" as the project directory name.'.
              format(path))
        return 'Game'
    else:
        # If we add a section header and change the comment prefix, then
        # bootstrap.cfg looks like an ini file and we can use ConfigParser.
        ini_str = '[default]\n' + open(path, 'r').read()
        ini_str = ini_str.replace('\n--', '\n#')
        ini_fp = StringIO(ini_str)
        config = configparser.RawConfigParser()
        config.read_file(ini_fp)

        game_directory_name = config.get('default', 'sys_game_folder')

        platform_mapping = {"win32": "pc", "darwin": "osx_gl"}
        user_settings_path = os.path.join(root_directory_path, 'Cache', game_directory_name,
                                          platform_mapping[sys.platform], 'User', 'AWS', 'user-settings.json')

        return user_settings_path


def __get_default_profile(args):
    user_settings_path = __get_user_settings_path()
    profile = 'default'

    if os.path.isfile(user_settings_path):
        with open(user_settings_path, 'r') as file:
            settings = json.load(file)
            profile = settings.get('DefaultProfile', args.profile)
    else:
        print('WARNING: No user_settings at {} - assuming credentials profile {}'.format(user_settings_path, profile))
    return profile


if __name__ == '__main__':
    main()
