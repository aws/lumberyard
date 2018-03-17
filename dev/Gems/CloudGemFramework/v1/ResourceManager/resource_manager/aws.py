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
import random
import time
from resource_manager_common import constant
import util
import json
import datetime
import botocore
from boto3 import Session
from botocore.exceptions import ProfileNotFound
from botocore.exceptions import ClientError
from botocore.exceptions import EndpointConnectionError
from botocore.exceptions import IncompleteReadError
from botocore.exceptions import ConnectionError
from botocore.exceptions import BotoCoreError
from botocore.exceptions import UnknownEndpointError
from errors import HandledError
from ConfigParser import RawConfigParser
from StringIO import StringIO
from config import CloudProjectSettings

from botocore.client import Config

from cgf_utils import json_utils

class AwsCredentials(object):
    """Wraps a RawConfigParser to treat a section named 'default' as a nomral section."""

    __REAL_DEFAULT_SECTION_NAME = 'default'
    __TEMP_DEFAULT_SECTION_NAME = constant.DEFAULT_SECTION_NAME
    __REAL_DEFAULT_SECTION_HEADING = '[' + __REAL_DEFAULT_SECTION_NAME + ']'
    __TEMP_DEFAULT_SECTION_HEADING = '[' + __TEMP_DEFAULT_SECTION_NAME + ']'

    def __init__(self):
        self.__config = RawConfigParser()

    def read(self, path):

        with open(path, 'r') as file:
            content = file.read()

        content = content.replace(self.__REAL_DEFAULT_SECTION_HEADING, self.__TEMP_DEFAULT_SECTION_HEADING)

        content_io = StringIO(content)

        self.__config.readfp(content_io)

    def write(self, path):

        content_io = StringIO()

        self.__config.write(content_io)

        content = content_io.getvalue()

        content = content.replace(self.__TEMP_DEFAULT_SECTION_HEADING, self.__REAL_DEFAULT_SECTION_HEADING)

        with open(path, 'w') as file:
            file.write(content)

    def __to_temp_name(self, name):

        if name == self.__REAL_DEFAULT_SECTION_NAME:
            name = self.__TEMP_DEFAULT_SECTION_NAME

        return name

    def __to_real_name(self, name):

        if name == self.__TEMP_DEFAULT_SECTION_NAME:
            name = self.__REAL_DEFAULT_SECTION_NAME

        return name

    def sections(self):
        sections = self.__config.sections()
        sections = [self.__to_real_name(section) for section in sections]
        return sections;

    def add_section(self, section):
        section = self.__to_temp_name(section)
        self.__config.add_section(section)

    def has_section(self, section):
        section = self.__to_temp_name(section)
        return self.__config.has_section(section)

    def options(self, section):
        section = self.__to_temp_name(section)
        return self.__config.options(section)

    def has_option(self, section, option):
        section = self.__to_temp_name(section)
        return self.__config.has_option(section, option)

    def get(self, section, option):
        section = self.__to_temp_name(section)
        return self.__config.get(section, option)

    def items(self, section):
        section = self.__to_temp_name(section)
        return self.__config.items(section)

    def set(self, section, option, value):
        section = self.__to_temp_name(section)
        self.__config.set(section, option, value)

    def remove_option(self, section, option):
        section = self.__to_temp_name(section)
        return self.__config.remove_option(section, section, option)

    def remove_section(self, section):
        section = self.__to_temp_name(section)
        return self.__config.remove_section(section)

class ClientWrapper(object):

    def __init__(self, wrapped_client, verbose):
        self.__wrapped_client = wrapped_client
        self.__client_type = type(wrapped_client).__name__
        self.__verbose = verbose

    @property
    def verbose(self):
        return self.__verbose

    @verbose.setter
    def verbose(self, value):
        self.__verbose = value

    @property
    def client_type(self):
        return self.__client_type

    def __getattr__(self, attr):
        orig_attr = self.__wrapped_client.__getattribute__(attr)
        if callable(orig_attr) and self.__verbose:
            def logging_wrapper(*args, **kwargs):
                self.__log_attempt(attr, args, kwargs)
                try:
                    result = orig_attr(*args, **kwargs)
                    self.__log_success(attr, result)
                    return result
                except Exception as e:
                    self.__log_failure(attr, e)
                    raise e
            return logging_wrapper
        else:
            return orig_attr

    def log(self, method_name, log_msg):

        msg = '\nAWS '
        msg += self.__client_type
        msg += '.'
        msg += method_name
        msg += ' '
        msg += log_msg
        msg += '\n'

        print msg


    def __log_attempt(self, method_name, args, kwargs):

        msg = 'attempt: '
        comma_needed = False
        for arg in args:
            if comma_needed: msg += ', '
            msg += type(arg).__name__
            msg += str(arg)
            comma_needed = True

        for key,value in kwargs.iteritems():
            if comma_needed: msg += ', '
            msg += key
            msg += '='
            msg += type(value).__name__
            if isinstance(value, basestring):
                msg += '"'
                msg += value
                msg += '"'
            elif isinstance(value, dict):
                msg += json.dumps(value, cls=json_utils.SafeEncoder)
            else:
                msg += str(value)
            comma_needed = True

        self.log(method_name, msg)


    def __log_success(self, method_name, result):

        msg = 'success: '
        msg += type(result).__name__
        if isinstance(result, dict):
            msg += json.dumps(result, cls=json_utils.SafeEncoder)
        else:
            msg += str(result)

        self.log(method_name, msg)


    def __log_failure(self, method_name, e):

        msg = ' failure '
        msg += type(e).__name__
        msg += ': '
        msg += str(getattr(e, 'response', e.message))

        self.log(method_name, msg)


class CloudFormationClientWrapper(ClientWrapper):

    BACKOFF_BASE_SECONDS = 0.1
    BACKOFF_MAX_SECONDS = 20.0
    BACKOFF_MAX_TRYS = 5

    def __init__(self, wrapped_client, verbose):
        super(CloudFormationClientWrapper, self).__init__(wrapped_client, verbose)

    def __getattr__(self, attr):
        orig_attr = super(CloudFormationClientWrapper, self).__getattr__(attr)
        if callable(orig_attr):

            def __try_with_backoff(*args, **kwargs):
                # http://www.awsarchitectureblog.com/2015/03/backoff.html
                backoff = self.BACKOFF_BASE_SECONDS
                count = 1
                while True:
                    try:
                        return orig_attr(*args, **kwargs)
                    except (ClientError,EndpointConnectionError,IncompleteReadError,ConnectionError,BotoCoreError,UnknownEndpointError) as e:
                        if count == self.BACKOFF_MAX_TRYS or (hasattr(e, 'response') and e.response and e.response['Error']['Code'] != 'Throttling'):
                            raise

                        backoff = min(self.BACKOFF_MAX_SECONDS, random.uniform(self.BACKOFF_BASE_SECONDS, backoff * 3.0))
                        if self.verbose:
                            self.log(attr, 'throttled attempt {}. Sleeping {} seconds'.format(count, backoff))
                        time.sleep(backoff)
                        count += 1

            return __try_with_backoff

        else:
            return orig_attr


class AWSContext(object):

    def __init__(self, context):
        self.__context = context
        self.__default_profile = None
        self.__session = None

    def initialize(self, args):
        self.__args = args

    def __init_session(self):

        if self.__args.aws_access_key or self.__args.aws_secret_key:

            if self.__args.profile:
                raise HandledError('Either the --profile or the --aws-secret-key and --aws-access-key options can be specified.')
            if not self.__args.aws_access_key or not self.__args.aws_secret_key:
                raise HandledError('Both --aws-secret-key and --aws-access-key are required if one if either one is given.')

            self.__session = Session(
                aws_access_key_id = self.__args.aws_access_key,
                aws_secret_access_key = self.__args.aws_secret_key)

        else:
            
            if self.__args.profile:
                if self.__args.aws_access_key or self.__args.aws_secret_key:
                    raise HandledError('Either the --profile or the --aws-secret-key and --aws-access-key options can be specified.')

            profile = self.__args.profile or self.__default_profile or 'default'
            
            if self.__args.verbose:
                self.__context.view.using_profile(profile)
            
            try:
                self.__session = Session(profile_name = profile)
            except (ProfileNotFound) as e:
                if self.has_credentials_file():
                    raise HandledError('The AWS session failed to locate AWS credentials for profile {}. Ensure that an AWS profile is present with command \'lmbr_aws list-profiles\' or using the Credentials Manager (AWS -> Credentials manager) in Lumberyard.  The AWS error message is \'{}\''.format(profile, e.message))
                try:
                    # Try loading from environment
                    self.__session = Session()
                except (ProfileNotFound) as e:
                    raise HandledError('The AWS session failed to locate AWS credentials from the environment. Ensure that an AWS profile is present with command \'lmbr_aws list-profiles\' or using the Credentials Manager (AWS -> Credentials manager) in Lumberyard.  The AWS error message is \'{}\''.format(e.message))

        self.__add_cloud_canvas_attribution(self.__session)


    def assume_role(self, logical_role_id, deployment_name):

        duration_seconds = 3600 # TODO add an option for this? Revist after adding GUI support.
        credentials = self.get_temporary_credentails(logical_role_id, deployment_name, duration_seconds)

        self.__session = Session(
            aws_access_key_id = credentials.get('AccessKeyId'), 
            aws_secret_access_key = credentials.get('SecretAccessKey'),
            aws_session_token = credentials.get('SessionToken'))

        self.__add_cloud_canvas_attribution(self.__session)


    def __add_cloud_canvas_attribution(self, session):
        if session._session.user_agent_extra is None:
            session._session.user_agent_extra = '/Cloud Canvas'
        else:
            session._session.user_agent_extra += '/Cloud Canvas'


    def __get_role_arn(self, role_logical_id, deployment_name):

        role_path = self.__context.config.get_project_stack_name()

        if self.__find_role_in_template(role_logical_id, self.__context.config.deployment_access_template_aggregator.effective_template):

            if deployment_name is None:
                deployment_name = self.__context.config.default_deployment

            if deployment_name is None:
                raise HandledError('The deployment access role {} was specified, but no deployment was given and there is no default deployment set for the project.'.format(role_logical_id))

            stack_arn = self.__context.config.get_deployment_access_stack_id(deployment_name)

            role_path = role_path + '/' + deployment_name

        elif self.__find_role_in_template(role_logical_id, self.__context.config.project_template_aggregator.effective_template):

            stack_arn = self.__context.config.project_stack_id

            deployment_name = None

        else:

            raise HandledError('Role could not find role {} in the project or deployment access templates.'.format(role_logical_id))

        role_physical_id = self.__context.stack.get_physical_resource_id(stack_arn, role_logical_id)
        account_id = util.get_account_id_from_arn(stack_arn)
        role_arn = 'arn:aws:iam::{}:role/{}/{}'.format(account_id, role_path, role_physical_id)

        self.__context.view.using_role(deployment_name, role_logical_id, role_physical_id)

        return role_arn
        

    def __find_role_in_template(self, logical_role_id, template):
        resources = template.get('Resources', {})
        definition = resources.get(logical_role_id, None)
        if definition:
            if definition.get('Type', '') == 'AWS::IAM::Role':
                return True
        return False


    def client(self, service_name, region = None, use_role = True):

        if self.__session is None:
            self.__init_session()

        if use_role:
            session = self.__session
        else:
            session = self.__session_without_role

        client = session.client(service_name, region_name = region, config=Config(signature_version='s3v4'))
        if service_name == 'cloudformation':
            wrapped_client = CloudFormationClientWrapper(client, self.__args.verbose)
        else:
            wrapped_client = ClientWrapper(client, self.__args.verbose)
        return wrapped_client


    @property
    def session(self):
        if self.__session is None:
            self.__init_session()
        return self.__session

    def set_default_profile(self, profile):
        self.__default_profile = profile
        self.__session = None

    def get_default_profile(self):
        return self.__default_profile

    def load_credentials(self):
        credentials = AwsCredentials()
        path = self.get_credentials_file_path()
        if os.path.isfile(path):
            self.__context.view.loading_file(path)
            credentials.read(path)
        return credentials

    def has_credentials_file(self):
        return os.path.isfile(self.get_credentials_file_path())

    def get_temporary_credentails(self, logical_role_id, deployment_name, duration_seconds):

        if logical_role_id:

            role_arn = self.__get_role_arn(logical_role_id, deployment_name)

            sts_client = self.client('sts')
            res = sts_client.assume_role(RoleArn=role_arn, RoleSessionName='lmbr_aws', DurationSeconds=duration_seconds)

            credentials = res.get('Credentials', {})

        else:

            sts_client = self.client('sts')
            res = sts_client.get_session_token(DurationSeconds=duration_seconds)

            credentials = res.get('Credentials', {})

        return credentials


    def save_credentials(self, credentials):
        path = self.get_credentials_file_path()
        dir = os.path.dirname(path)
        if not os.path.isdir(dir):
            os.makedirs(dir)
        self.__context.view.saving_file(path)
        credentials.write(path)

    def get_credentials_file_path(self):
        return os.path.join(os.path.expanduser('~'), '.aws', 'credentials')

    def profile_exists(self, profile):
        credentials = self.load_credentials()
        return credentials.has_section(profile)
