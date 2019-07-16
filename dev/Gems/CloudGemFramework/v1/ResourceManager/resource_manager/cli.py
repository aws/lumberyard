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

import argparse
import os
import traceback
import sys
import json
import imp

import project
import deployment
import mappings
import resource_group
import player_identity
import profile
import importer
import gem
import log_finder
import backup

from resource_manager_common import constant
import security
import function

from config import ConfigContext
from errors import HandledError
from context import Context
from metrics import MetricsContext
from util import Args

from botocore.exceptions import NoCredentialsError
from botocore.exceptions import EndpointConnectionError
from botocore.exceptions import IncompleteReadError
from botocore.exceptions import ConnectionError
from botocore.exceptions import UnknownEndpointError

def main():

    '''Main entry point for the lmbr_aws command line interface'''

    args = None

    try:

        metricsInterface = MetricsContext('cli')
        context = Context(metricsInterface)

        __bootstrap_context(context)

        # Deprecated in 1.9. TODO: remove.
        context.hooks.call_module_handlers('cli-plugin-code/resource_commands.py', 'add_cli_view_commands',
            args=[context.view],
            deprecated=True)

        context.hooks.call_module_handlers('resource-manager-code/command.py', 'add_cli_view_commands',
            kwargs={
                'view_context': context.view
            }
        )

        parser = argparse.ArgumentParser(
            prog = 'lmbr-aws',
            description='Manage AWS resources used by a Lumberyard project.'
        )

        parser.register('action', 'parsers', AliasedSubParsersAction)

        subparsers = parser.add_subparsers(metavar='COMMAND')

        __add_built_in_commands(context, subparsers)
        __add_hook_module_commands(context, subparsers)
        try:
            args = Args(**parser.parse_args().__dict__)
        except:
            metricsInterface.submit_command_error()
            return constant.CLI_RETURN_ERROR_HANDLED_CODE

        command_name = ''
        for arg in sys.argv[1:]:
            if arg.startswith('-'):
                break
            if command_name:
                command_name = command_name + '-'
            command_name = command_name + arg

        metricsInterface.set_command_name(command_name)
        metricsInterface.submit_attempt()

        try:

            context.initialize(args)

            if args.func != project.update_framework_version:
                context.config.verify_framework_version()

            # Using context_func instead of func to specify the cli command handler
            # causes the function to be called with only args using **kwargs.
            #
            # It is expected that the value is a function on an object instance that
            # already access to context, so that argument is not passed. Python takes
            # care of passing self to the function when we call it.
            #
            # Using **kwargs makes the function easier to call from other code. Use
            # dest= in the add_option call to change the name of the parameter passed
            # to the handler if you don't like the default.
            #
            # See the gem module for examples.

            if args.context_func:
                args.context_func(**__remove_common_args(args))
            else:
                args.func(context, args)

            metricsInterface.submit_success()

        except KeyboardInterrupt:
            metricsInterface.submit_interrupted()
            raise
        except:
            metricsInterface.submit_failure()
            raise

        return constant.CLI_RETURN_OK_CODE

    except KeyboardInterrupt:
        return constant.CLI_RETURN_ERROR_HANDLED_CODE
    except HandledError as e:
        print '\nERROR: {0}'.format(e)
        if '--verbose' in sys.argv:
            traceback.print_exc()
        return constant.CLI_RETURN_ERROR_HANDLED_CODE
    except NoCredentialsError:
        print '\nERROR: No AWS credentials were provided.'
        if '--verbose' in sys.argv:
            traceback.print_exc()
        return constant.CLI_RETURN_ERROR_HANDLED_CODE
    except (EndpointConnectionError, IncompleteReadError, ConnectionError, UnknownEndpointError) as e:
        print '\nERROR: We were unable to contact your AWS endpoint.\n {0}'.format(e.message)
        if '--verbose' in sys.argv:
            traceback.print_exc()
        return constant.CLI_RETURN_ERROR_HANDLED_CODE
    except Exception as e:
        print '\nERROR: An unexpected error has occured:\n'
        traceback.print_exc()
        return constant.CLI_RETURN_ERROR_UNHANDLED_CODE

def __bootstrap_context(context):
    '''Process a select subset of the command line arguments in order to pre-initialize the context.

    This is done so that command line plugins can be located in the file system at locations that
    can be overridden by command line arguments.
    '''
    parser = argparse.ArgumentParser(add_help=False)
    __add_bootstrap_args(parser)
    args, unknown = parser.parse_known_args()
    context.bootstrap(Args(**args.__dict__))


def __remove_common_args(args):
    '''Return a dict with the keys/values from an Args object that are not system defined arguments.'''
    return { key: value for key, value in args.__dict__.items() if key not in COMMON_ARG_KEYS }


# this set should contain all the args added by __add_common_args and __add_bootstrap_args,
# plus any other propeties added to the args object as configured by the parsers
COMMON_ARG_KEYS = set([

    # these are added by various parser configurations
    'subparser_name',
    'func',
    'context_func',

    # these come from __add_common_args
    'aws_access_key',
    'aws_secret_key',
    'profile',
    'assume_role',

    # these come from __add_bootstrap_args
    'root_directory',
    'game_directory',
    'aws_directory',
    'user_directory',
    'verbose',
    'no_prompt',
    'region_override',
    'only_cloud_gems'
    ])


def __add_common_args(parser, no_assume_role = False):
    '''These arguments are accepted by most sub-commands.'''
    parser.add_argument('--aws-access-key', help='The AWS access key to use. The default value is taken from the ~/.aws/credentials file.')
    parser.add_argument('--aws-secret-key', help='The AWS secret key to use. The default value is taken from the ~/.aws/credentials file.')
    parser.add_argument('--profile', '-P', help='The AWS configuration profile in the ~/.aws/credentials and ~/.aws/config files to use.')
    if not no_assume_role:
        parser.add_argument('--assume-role', '-R', metavar='ROLE-NAME', help='Specifies an IAM role that will be assumed when performing the requested actions. The credentials taken from the ~/.aws/credentials file must be able to asssume this role.')
    __add_bootstrap_args(parser)


def __add_bootstrap_args(parser):
    '''These arguments are used to pre-initialize the context.'''
    parser.add_argument('--root-directory', default=os.getcwd(), help='Lumberyard install directory and location of bootstrap.cfg file. Default is the current working directory.')
    parser.add_argument('--game-directory', help='Location of the game project directory. The default is {root-directory}\{game} where {game} is determined by the sys_game_folder setting in the {root-directory}\bootstrap.cfg file.')
    parser.add_argument('--aws-directory', help='Location of AWS configuration directory for the game. The default is {game-directory}\AWS.')
    parser.add_argument('--user-directory', help='Location of user cache directory. The default is {root-directory}\Cache\{game}\AWS where {game} is determined by the sys_game_folder setting in the {root-directory}\bootstrap.cfg file.')
    parser.add_argument('--verbose', action='store_true', help='Show additional output when executing commands.')
    parser.add_argument('--no-prompt', action='store_true', help='Special flag set automatically when entering from tests - calls which would raise an option for user input will instead raise an error')
    parser.add_argument('--region-override', help='An override to manually indicate which region to use in your local-project-setting.json other than the default.')
    parser.add_argument('--only-cloud-gems', nargs="+", help='Only modify the following cloud gems.')


def __add_project_stack_commands(stack_subparser):
    stack_subparser.register('action', 'parsers', AliasedSubParsersAction)
    subparsers = stack_subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = subparsers.add_parser('create', help='Creates the AWS resources needed for a Lumberyard project. If the {game}\AWS directory contains no resource definitions, default ones will be created.')
    subparser.add_argument('--stack-name', help='The name used for the project stack. The default is the name of the {game} directory.')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security changes.')
    subparser.add_argument('--files-only', action='store_true', help='Initializes the {game}\AWS directory and exit. If this option is given the project stack is not created. If the directory already exists and contains any files, no new files are created.')
    subparser.add_argument('--region', required=True, help='The AWS region where the project stack will be located.')
    subparser.add_argument('--record-cognito-pools', action='store_true', help='Record the cognito pools that are stood up during deployment access updates')
    __add_common_args(subparser, no_assume_role = True)
    subparser.set_defaults(func=project.create_stack)

    subparser = subparsers.add_parser('upload-resources', aliases=['upload', 'update'], help='Updates the AWS resources used by Lumberyard project.')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security related changes.')
    __add_common_args(subparser)
    subparser.set_defaults(func=project.update_stack)

    subparser = subparsers.add_parser('delete', help='Deletes the AWS resources used by Lumberyard project. This command will not delete projects with deployments.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    __add_common_args(subparser)
    subparser.set_defaults(func=project.delete_stack)

    subparser = subparsers.add_parser('list-resources')
    subparser.add_argument('--show-id', action='store_true', help='Include the AWS resource id in the output.')
    __add_common_args(subparser)
    subparser.set_defaults(func=project.list_project_resources)

    subparser = subparsers.add_parser('create-extension-template')
    subparser.add_argument('--project', action='store_true', help='Create a project-template-extensions.json file in the project directory.')
    subparser.add_argument('--deployment', action='store_true', help='Create a deployment-template-extensions.json file in the project directory.')
    subparser.add_argument('--deployment-access', action='store_true', help='Create a deployment-access-template-extensions.json file in the project directory.')
    __add_common_args(subparser)
    subparser.set_defaults(func=project.create_extension_template)

    subparser = subparsers.add_parser('update-framework-version')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security related changes.')
    __add_common_args(subparser)
    subparser.set_defaults(func=project.update_framework_version)

    subparser = subparsers.add_parser('clean-custom-resources', help='Deletes unreferenced versions of custom resource lambdas.')
    __add_common_args(subparser)
    subparser.set_defaults(func=project.clean_custom_resource_handlers)


def __add_resource_group_commands(group_subparser):
    group_subparser.register('action', 'parsers', AliasedSubParsersAction)
    subparsers = group_subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = subparsers.add_parser('disable', help='Disable a resource group. Disabled resource groups are not be included in deployments.')
    subparser.add_argument('--resource-group', '-r', required=True, metavar='GROUP', help='The name of the resource group.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.disable)

    subparser = subparsers.add_parser('enable', help='Enable a resource group.')
    subparser.add_argument('--resource-group', '-r', required=True, metavar='GROUP', help='The name of the resource group.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.enable)

    subparser = subparsers.add_parser('list-resources', help='list all the resources for the specified resource group')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', help='The name of the deployment that contains the resource group. If not specified, the default deployment is used.')
    subparser.add_argument('--resource-group', '-r', required=True, help='The name of the resource group to describe')
    subparser.add_argument('--show-id', action='store_true', help='Include the AWS resource id in the output.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.list_resource_group_resources)

    subparser = subparsers.add_parser('list', help='List the project''s resource groups.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', help='The name of the deployment used when determining the resource group status. If not specified, the default deployment is used.')
    subparser.add_argument('--show-id', action='store_true', help='Include the AWS resource id in the output.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.list)

    subparser = subparsers.add_parser('upload-resources', aliases=['upload', 'update'], help='Uploads and applies changes made to local resource-template.json files.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', help='The deployment to update. If not specified the default deployment is updated.')
    subparser.add_argument('--resource-group', '-r', metavar='GROUP', help='The name of the resource group to update. If not specified, all the resource groups in the deployment are updated.')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security related changes.')
    subparser.add_argument('--record-cognito-pools', action='store_true', help='Record the cognito pools that are stood up during deployment access updates')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.upload_resources)

    subparser = subparsers.add_parser('add-player-access', help='Add player access to a resource.')
    subparser.add_argument('--resource-group', '-r', metavar='GROUP', required=True, help='The name of the resource group.')
    subparser.add_argument('--resource', metavar='RESOURCE', required=True, help='The name of the resource.')
    subparser.add_argument('--action', metavar='ACTION', required=True, nargs='+', help='The action to be added.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.add_player_access)


def __add_deployment_commands(deployment_subparser):
    deployment_subparser.register('action', 'parsers', AliasedSubParsersAction)
    subparsers = deployment_subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = subparsers.add_parser('create', help='Create a complete and independent copy of all the resources needed by the Lumberyard project.')
    subparser.add_argument('--deployment', '-d', required=True, metavar='DEPLOYMENT', help='The name of the deployment to add.')
    subparser.add_argument('--stack-name', help='The name used for the deployment stack. The default name is "ProjectStackName-DeploymentName".')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will modify AWS security configuration.')
    subparser.add_argument('--make-project-default', action='store_true', help='After creation, the deployment will be set as the default deployment for the development builds of the project')
    subparser.add_argument('--make-release-deployment', action='store_true', help='After creation, the deployment will be set as the deployment for the release builds of the project')
    subparser.add_argument('--tags', nargs='+', required=False, help='Deployment tags, to allow per-deployment overrides')
    subparser.add_argument('--parallel', action='store_true', help='Deploy resource groups in parallel instead of one-at-a-time. (More likely to encounter resource limits.)')
    subparser.add_argument('--record-cognito-pools', action='store_true', help='Record the cognito pools that are stood up during deployment access updates')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.create_stack)

    subparser = subparsers.add_parser('delete', help='Delete a complete and independent copy of all the resources needed by the Lumberyard project.')
    subparser.add_argument('--deployment', '-d', required=True, metavar='DEPLOYMENT', help='The name of the deployment to delete.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    subparser.add_argument('--parallel', action='store_true', help='Delete resource groups in parallel instead of one-at-a-time. (More likely to encounter resource limits.)')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.delete_stack)

    subparser = subparsers.add_parser('list-resources', help='list all the resources for the specified deployment')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', help='The name of the deployment to describe. If not specified, the default deployment is used.')
    subparser.add_argument('--show-id', action='store_true', help='Include the AWS resource id in the output.')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.list_deployment_resources)

    subparser = subparsers.add_parser('default', help='set, clear, or show the default deployment to be used during development.')
    group = subparser.add_mutually_exclusive_group()
    group.add_argument('--set', metavar='DEPLOYMENT', help='Set the default to the provided deployment name. ')
    group.add_argument('--clear', action='store_true', help='Clear the default. ')
    group.add_argument('--show', action='store_true', help='Show the defaults. ')
    subparser.add_argument('--project', action='store_true', help='Applies --set and --clear to the project default instead of the user default. Ignored for --show.')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.default)

    subparser = subparsers.add_parser('release', help= 'set, clear, or show the deployment to be used by release builds of the game')
    group = subparser.add_mutually_exclusive_group()
    group.add_argument('--set', metavar='DEPLOYMENT', help='Set the default to the provided deployment name. ')
    group.add_argument('--clear', action='store_true', help='Clear the default. ')
    group.add_argument('--show', action='store_true', help='Show the defaults. ')
    subparser.add_argument('--project', action='store_true', help='Applies --set and --clear to the project default instead of the user default. Ignored for --show.')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.release)

    subparser = subparsers.add_parser('protect', help='Sets the protection flag on a deployment so that the user will be warned if they try to run a protected deployment from a test or development client. Protected deployments will help reduce development code inadvertently impacting customer facing resources.')
    group = subparser.add_mutually_exclusive_group()
    group.add_argument('--set', metavar='DEPLOYMENT', help='Set the provided deployment name to be a protected deployment. ')
    group.add_argument('--clear', metavar='DEPLOYMENT', help='Remove the protected status from the provided deployment name. ')
    group.add_argument('--show', action='store_true', help='Show the list of currently protected deployments. ')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.protect)

    subparser = subparsers.add_parser('list', help='List all deployments in the local project.')
    subparser.add_argument('--show-id', action='store_true', help='Include the AWS resource id in the output.')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.list)

    subparser = subparsers.add_parser('upload-resources', aliases=['upload', 'update'], help='Uploads and applies changes made to local resource-template.json files.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', help='The deployment to update. If not specified the default deployment is updated.')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security changes.')
    subparser.add_argument('--parallel', action='store_true', help='Update resource groups in parallel instead of one-at-a-time. (More likely to encounter resource limits.)')
    subparser.add_argument('--record-cognito-pools', action='store_true', help='Record the cognito pools that are stood up during deployment access updates')

    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.upload_resources)

    subparser = subparsers.add_parser('update-access', help='Updates a deployment access stack or stacks.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='The name of the deployment whos access stack is updated. If omitted, the default deployment is updated. Use * to update all deployments.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security changes.')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.update_access_stack)

    subparser = subparsers.add_parser('tags', help='updates a deployments tags, which are used to provide resource-group overrides to a deployment')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='The name of the deployment whos tags will be updated')
    subparser.add_argument('--add', nargs='+', required=False, help='The tags to add to the deployment')
    subparser.add_argument('--delete', nargs='+', required=False, help='The tags to delete from the deployment')
    subparser.add_argument('--clear', action='store_true', required=False, help='Clear all tags for the deployment')
    subparser.add_argument('--list', action='store_true', required=False, help='List all tags on a deployment')
    subparser.set_defaults(func=deployment.tags)


def __add_profile_commands(profile_subparser):
    profile_subparser.register('action', 'parsers', AliasedSubParsersAction)
    subparsers = profile_subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = subparsers.add_parser('add', help='Add an AWS profile.')
    subparser.add_argument('--aws-access-key', required=True, help='The AWS access key associated with the added profile.')
    subparser.add_argument('--aws-secret-key', required=True, help='The AWS secret key associated with the added profile.')
    subparser.add_argument('--profile', required=True, help='The name of the AWS profile to add.')
    subparser.add_argument('--make-default', required=False, action='store_true')
    subparser.set_defaults(func=profile.add)

    subparser = subparsers.add_parser('update', help='Update an AWS profile.')
    subparser.add_argument('--aws-access-key', required=False, help='The AWS access key associated with the updated profile. Default is to not change the AWS access key associated with the profile.')
    subparser.add_argument('--aws-secret-key', required=False, help='The AWS secret key associated with the added profile. Default is to not change the AWS secret key associated with the profile.')
    subparser.add_argument('--profile', required=True, help='The name of the AWS profile to add.')
    subparser.set_defaults(func=profile.update)

    subparser = subparsers.add_parser('remove', help='Remove an AWS profile.')
    subparser.add_argument('--profile', required=True, help='The name of the AWS profile to add.')
    subparser.set_defaults(func=profile.remove)

    subparser = subparsers.add_parser('rename', help='Rename an AWS profile.')
    subparser.add_argument('--old-name', required=True, help='The name of the AWS profile to change.')
    subparser.add_argument('--new-name', required=True, help='The new name of the AWS profile.')
    subparser.set_defaults(func=profile.rename)

    subparser = subparsers.add_parser('list', help='Lists the AWS profiles that have been configured.')
    subparser.add_argument('--profile', required=False, help='The name of the profile to list. All profies are listed by default.')
    subparser.set_defaults(func=profile.list)

    subparser = subparsers.add_parser('default', help='Set the default AWS profile.')
    group = subparser.add_mutually_exclusive_group()
    group.add_argument('--set', metavar='PROFILE', help='Set the default profile to the provided deployment name. ')
    group.add_argument('--clear', action='store_true', help='Clear the default. ')
    group.add_argument('--show', action='store_true', help='Show the defaults. ')
    __add_common_args(subparser)
    subparser.set_defaults(func=profile.default)


def __add_login_provider_commands(login_subparser):
    login_subparser.register('action', 'parsers', AliasedSubParsersAction)
    subparsers = login_subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = subparsers.add_parser('add', help='Adds a login provider to your Player Access template, so that you can connect your application to cognito-identity.')
    subparser.add_argument('--provider-name', required=True, help='The name of the provider e.g. amazon, google, facebook. If you are using a generic openid provider it must be uniquely named here and the provider-path, provider-uri, provider-port options must be provided.')
    subparser.add_argument('--app-id', required=True, help='The application id from your login provider (this is usually different from your client id though not always). This is the value that would go into your cognito-identity configuration.')
    subparser.add_argument('--client-id', required=True, help='The unique application client id for the login provider.')
    subparser.add_argument('--client-secret', required=True, help='The secret key to use with your login provider.')
    subparser.add_argument('--redirect-uri', required=True, help='The redirect URI configured with your auth provider.')
    subparser.add_argument('--provider-uri', required=False, help='The URI for a generic openid connect provider. This option is only used for generic openid providers.')
    subparser.add_argument('--provider-port', required=False, help='The port your provider listens on for their api. This option is only used for generic openid providers.')
    subparser.add_argument('--provider-path', required=False, help='The path portion of your provider\'s uri. This option is only used for generic openid providers.')
    __add_common_args(subparser)
    subparser.set_defaults(func=player_identity.add_login_provider)

    subparser = subparsers.add_parser('update', help='Updates an existing login provider to your Player Access template, so that you can connect your application to cognito-identity.')
    subparser.add_argument('--provider-name', required=True, help='The name of the provider e.g. amazon, google, facebook. If you are using a generic openid provider its unique name must be specified here and the provider-path, provider-uri, and/or provider-port options can be provided.')
    subparser.add_argument('--app-id', required=False, help='The application id from your login provider (this is usually different from your client id though not always). This is the value that would go into your cognito-identity configuration.')
    subparser.add_argument('--client-id', required=False, help='The unique application client id for the login provider.')
    subparser.add_argument('--client-secret', required=False, help='The secret key to use with your login provider.')
    subparser.add_argument('--redirect-uri', required=False, help='The redirect URI configured with your auth provider.')
    subparser.add_argument('--provider-uri', required=False, help='The URI for a generic openid connect provider. This option is only use for generic openid providers.')
    subparser.add_argument('--provider-port', required=False, help='The port your provider listens on for their api. This option is only used for generic openid providers.')
    subparser.add_argument('--provider-path', required=False, help='The path portion of your provider\'s uri. This option is only used for generic openid providers.')
    __add_common_args(subparser)
    subparser.set_defaults(func=player_identity.update_login_provider)

    subparser = subparsers.add_parser('remove', help='Removes a login provider from your Player Access template.')
    subparser.add_argument('--provider-name', required=True, help='The name of the provider e.g. amazon, google, facebook; or the unique name of a generic openid provider.')
    __add_common_args(subparser)
    subparser.set_defaults(func=player_identity.remove_login_provider)


def __add_resource_importer_commands(import_subparser):
    import_subparser.register('action', 'parsers', AliasedSubParsersAction)
    subparsers = import_subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = subparsers.add_parser('list-importable-resources', help='List all supported resources currently existing on AWS.')
    subparser.add_argument('--type', required = True, help='Type of the resource to list.')
    subparser.add_argument('--region', help='The AWS region of the resources. The default value is the region of the project stack if it exists.')
    __add_common_args(subparser)
    subparser.set_defaults(func = importer.list_aws_resources)

    subparser = subparsers.add_parser('import-resource', help = 'Import the resource to the resource group.')
    subparser.add_argument('--type', choices=['dynamodb', 's3', 'lambda', 'sns', 'sqs'], help='Type of the resource to import.')
    subparser.add_argument('--arn', required = True, help='ARN of the AWS resource to import.')
    subparser.add_argument('--resource-name', required = True, help='The name used for the resource in the resource group.')
    subparser.add_argument('--resource-group', '-r', required = True, help='The resource group to import.')
    subparser.add_argument('--download', action='store_true', help='Choose whether to download the contents of the S3 bucket.')
    __add_common_args(subparser)
    subparser.set_defaults(func = importer.import_resource)


def __add_parameter_commands(parameter_subparser):
    parameter_subparser.register('action', 'parsers', AliasedSubParsersAction)
    subparsers = parameter_subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = subparsers.add_parser('list', help='Lists the parameter values used when updating resource group stacks.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='Shows only parameters for the specified deployment. By default parameters for all deployments are shown.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP', required=False, help='Shows only parameters for the specified resource group. By default parameters for all resource groups are shown.')
    subparser.add_argument('--parameter', '-p', metavar='PARAMETER', required=False, help='Shows only the specified parameter. By default all parameters are shown.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.list_parameters)

    subparser = subparsers.add_parser('set', help='Sets a parameter value used when updating resource group stacks.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=True, help='Sets the parameter value for the specified deployment. Use * to set the parameter for all deployments.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP', required=True, help='Sets the parameter value for the specified resource group. Use * to set the parameter for all resource groups.')
    subparser.add_argument('--parameter', '-p', metavar='PARAMETER', required=True, help='The name of the parameter to set.')
    subparser.add_argument('--value', '-v', metavar='VALUE', required=True, help='The parameter\'s value.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.set_parameter)

    subparser = subparsers.add_parser('clear', help='Clears a parameter value used when updating resource group stacks.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='Clears the parameter value for the specified deployment. Use * to clear a parameter that has been set for all deployments. If ommited, the parameter will be cleared for deployments.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP', required=False, help='Clears the parameter value for the specified resource group. Use * to set the parameter for all resource groups. If ommitted the parameter will be cleared for all resource groups.')
    subparser.add_argument('--parameter', '-p', metavar='PARAMETER', required=True, help='The name of the parameter to clear.')
    subparser.add_argument('--confirm-clear', required=False, action='store_true', help='Confirms that you want to clear the parameter values. Required only if the --deployment or --resource-group option is omitted. By default you are prompted to confirm the change.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.clear_parameter)


def __add_mappings_commands(mappings_subparser):
    mappings_subparser.register('action', 'parsers', AliasedSubParsersAction)
    subparsers = mappings_subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = subparsers.add_parser('update', help='Update the logical to physical resource name mappings.')
    group = subparser.add_mutually_exclusive_group()
    group.add_argument('--release', required=False, action='store_true', help='Causes the release mappings to be updated. By default the mappings used during development are updated.')
    group.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='Updates the launcher mappings to use the selected deployment')
    subparser.add_argument('--ignore-cache', required=False, action='store_true', help="Ignores the cached mappings stored in the s3 configuration bucket, forcing a rebuild")
    __add_common_args(subparser)
    subparser.set_defaults(func=mappings.force_update)

    subparser = subparsers.add_parser('list', help='Show the logical to physical resource name mappings.')
    __add_common_args(subparser)
    subparser.set_defaults(func=mappings.list)


def __add_function_commands(function_subparser):
    function_subparser.register('action', 'parsers', AliasedSubParsersAction)
    subparsers = function_subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = subparsers.add_parser('upload-code', aliases=['upload', 'update'], help='Uploads and applies changes made to local resource-template.json files.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', help='The deployment to update. If not specified the default deployment is updated. Ignored if --project is specified.')
    group = subparser.add_mutually_exclusive_group(required = True)
    group.add_argument('--resource-group', '-r', metavar='GROUP', required = False, help='The name of the resource group that defines the function to be updated.')
    group.add_argument('--project', '-p', required = False, action='store_true', help='Indicates that a project defined is to be updated.')
    subparser.add_argument('--function', '-f', metavar='FUNCTION', help='The name of the lambda function to update. If not specified, all the Lambda functions in the resource group are updated.')
    subparser.add_argument('--keep', '-k', required = False, action='store_true', help='Keep the generated .zip file instead of deleting it after uploading.')
    __add_common_args(subparser)
    subparser.set_defaults(func=function.upload_lambda_code)

    subparser = subparsers.add_parser('get-log', help='Retrieves data from a CloudWatch log file.')
    subparser.add_argument('--log-stream-name', '-l', metavar='LOG-STREAM', required=False, help='The log stream name, or part of a log stream mane. If omitted, the most recent log stream will be shown.')
    subparser.add_argument('--function', '-f', metavar='FUNCTION', required=True, help='The logical name of a lambda function resource.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='The name of a deployment. If given then --resource-group must also be given. If ommitted, then the function must exist in the project stack.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP', required=False, help='The name of a resource group. If given then --deployment must also be given.')
    __add_common_args(subparser)
    subparser.set_defaults(func=function.get_function_log)

    subparser = subparsers.add_parser('create-folder',  help='Recreates the default function folder for a lambda function resource.')
    subparser.add_argument('--function', '-f', metavar='FUNCTION', required=True, help='The logical name of a lambda function resource.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP', required=True, help='The name of a resource group.')
    subparser.add_argument("--force", required=False, action='store_true', help='Skips checks for existence of resource and type of resource. Used when creating folders for to-be-created functions.')
    subparser.set_defaults(func=resource_group.create_function_folder)


def __add_log_finder_commands(log_subparser):
    log_subparser.add_argument('--deployment', '-d', required=False, metavar='DEPLOYMENT',
                               help='The deployment to search logs in. If not specified the default deployment is used')
    log_subparser.add_argument('--function', '-f', required=True, help="The logical name of the function you want to read the logs for. in the form <ResourceGroup>.<LogicalFunctionName>")
    log_subparser.add_argument('--request', required=True, help='The aws_request_id of the call you are looking for')
    log_subparser.set_defaults(func=log_finder.get_logs)

def __add_backup_commands(backup_subparser):
    backup_subparser.add_argument('--deployment', '-d', required=False, metavar='DEPLOYMENT',
                               help='The deployment to search logs in. If not specified the default deployment is used')
    backup_subparser.add_argument('--resource', '-r', required=False,
                                  help='The resource to backup, in <group>.<resource> format')
    backup_subparser.add_argument(
        '--type', '-t', required=False, help='The type of the resource.', choices=['ddb', 's3'])
    backup_subparser.add_argument(
        '--backup-name', '-b', required=False, help='The name of the backup.')
    backup_subparser.set_defaults(func=backup.backup_resource)


def __add_deprecated_commands(context, subparsers):
    '''Adds the built in sub-commands'''

    subparser = subparsers.add_parser('list-importable-resources')
    subparser.add_argument('--type', required = True, help='Type of the resource to list.')
    subparser.add_argument('--region', help='The AWS region of the resources. The default value is the region of the project stack if it exists.')
    __add_common_args(subparser)
    subparser.set_defaults(func = importer.list_aws_resources)

    subparser = subparsers.add_parser('import-resource')
    subparser.add_argument('--type', choices=['dynamodb', 's3', 'lambda', 'sns', 'sqs'], help='Type of the resource to import.')
    subparser.add_argument('--arn', required = True, help='ARN of the AWS resource to import.')
    subparser.add_argument('--resource-name', required = True, help='The name used for the resource in the resource group.')
    subparser.add_argument('--resource-group', '-r', required = True, help='The resource group to import.')
    subparser.add_argument('--download', action='store_true', help='Choose whether to download the contents of the S3 bucket.')
    __add_common_args(subparser)
    subparser.set_defaults(func = importer.import_resource)

    subparser = subparsers.add_parser('create-project-stack')
    subparser.add_argument('--stack-name', help='The name used for the project stack. The default is the name of the {game} directory.')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    subparser.add_argument('--files-only', action='store_true', help='Initializes the {game}\AWS directory and exit. If this option is given the project stack is not created. If the directory already exists and contains any files, no new files are created.')
    subparser.add_argument('--region', required=True, help='The AWS region where the project stack will be located.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security changes.')
    __add_common_args(subparser, no_assume_role = True)
    subparser.set_defaults(func=project.create_stack)

    subparser = subparsers.add_parser('update-project-stack')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security related changes.')
    __add_common_args(subparser)
    subparser.set_defaults(func=project.update_stack)

    subparser = subparsers.add_parser('delete-project-stack')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    __add_common_args(subparser)
    subparser.set_defaults(func=project.delete_stack)

    subparser = subparsers.add_parser('default-deployment')
    group = subparser.add_mutually_exclusive_group()
    group.add_argument('--set', metavar='DEPLOYMENT', help='Set the default to the provided deployment name. ')
    group.add_argument('--clear', action='store_true', help='Clear the default. ')
    group.add_argument('--show', action='store_true', help='Show the defaults. ')
    subparser.add_argument('--project', action='store_true', help='Applies --set and --clear to the project default instead of the user default. Ignored for --show.')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.default)

    subparser = subparsers.add_parser('release-deployment')
    group = subparser.add_mutually_exclusive_group()
    group.add_argument('--set', metavar='DEPLOYMENT', help='Set the release deployment to the provided deployment name. ')
    group.add_argument('--clear', action='store_true', help='Clear the release deployment. ')
    group.add_argument('--show', action='store_true', help='Show the release deployment. ')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.release)

    subparser = subparsers.add_parser('protect-deployment')
    group = subparser.add_mutually_exclusive_group()
    group.add_argument('--set', metavar='DEPLOYMENT', help='Set the provided deployment name to be a protected deployment. ')
    group.add_argument('--clear', metavar='DEPLOYMENT', help='Remove the protected status from the provided deployment name. ')
    group.add_argument('--show', action='store_true', help='Show the list of currently protected deployments. ')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.protect)

    subparser = subparsers.add_parser('upload-resources')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', help='The deployment to update. If not specified the default deployment is updated.')
    subparser.add_argument('--resource-group', '-r', metavar='GROUP', help='The name of the resource group to update. If not specified, all the resource groups in the deployment are updated.')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security related changes.')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.upload_resources)

    subparser = subparsers.add_parser('upload-lambda-code')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', help='The deployment to update. If not specified the default deployment is updated.')
    subparser.add_argument('--resource-group', '-r', metavar='GROUP', required = True, help='The name of the resource group to update. If not specified, all the resource groups in the deployment are updated.')
    subparser.add_argument('--function', metavar='FUNCTION', help='The name of the lambda function to update. If not specified, all the Lambda functions in the resource group are updated.')
    __add_common_args(subparser)
    subparser.set_defaults(func=function.upload_lambda_code)

    subparser = subparsers.add_parser('update-mappings')
    group = subparser.add_mutually_exclusive_group()
    group.add_argument('--release', required=False, action='store_true', help='Causes the release mappings to be updated. By default the mappings used during development are updated.')
    group.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='Updates the launcher mappings to use the selected deployment')
    subparser.add_argument('--ignore-cache', required=False, action='store_true', help="Ignores the cached mappings stored in the s3 configuration bucket, forcing a rebuild")
    __add_common_args(subparser)
    subparser.set_defaults(func=mappings.force_update)

    subparser = subparsers.add_parser('list-mappings')
    __add_common_args(subparser)
    subparser.set_defaults(func=mappings.list)

    subparser = subparsers.add_parser('list-resource-groups')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', help='The name of the deployment used when determining the resource group status. If not specified, the default deployment is used.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.list)

    subparser = subparsers.add_parser('create-deployment')
    subparser.add_argument('--deployment', '-d', required=True, metavar='DEPLOYMENT', help='The name of the deployment to add.')
    subparser.add_argument('--stack-name', help='The name used for the deployment stack. The default name is "ProjectStackName-DeploymentName".')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    subparser.add_argument('--make-project-default', action='store_true', help='After creation, the deployment will be set as the default deployment for the development builds of the project')
    subparser.add_argument('--make-release-deployment', action='store_true', help='After creation, the deployment will be set as the deployment for the release builds of the project')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security changes.')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.create_stack)

    subparser = subparsers.add_parser('delete-deployment')
    subparser.add_argument('--deployment', '-d', required=True, metavar='DEPLOYMENT', help='The name of the deployment to delete.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.delete_stack)

    subparser = subparsers.add_parser('list-deployments')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.list)

    subparser = subparsers.add_parser('list-resources')
    subparser.add_argument('--stack-id', help='The ARN of the stack to list resources for. Defaults to project, deployment, or resource group stack id as determined by the --deployment and --resource-group parameters.')
    subparser.add_argument('--deployment', '-d', help='The name of the deployment for which resources are listed. Defaults to listing all the project stack''s resources.')
    subparser.add_argument('--resource-group', '-r', help='The name of the resource group for which resources are listed. If specified then deployment must also be specified. Defaults to listing all the deployment''s or project''s resources.')
    __add_common_args(subparser)
    subparser.set_defaults(func=project.deprecated_list_resources)

    subparser = subparsers.add_parser('add-login-provider')
    subparser.add_argument('--provider-name', required=True, help='The name of the provider e.g. amazon, google, facebook. If you are using a generic openid provider it must be uniquely named here and the provider-path, provider-uri, provider-port options must be provided.')
    subparser.add_argument('--app-id', required=True, help='The application id from your login provider (this is usually different from your client id though not always). This is the value that would go into your cognito-identity configuration.')
    subparser.add_argument('--client-id', required=True, help='The unique application client id for the login provider.')
    subparser.add_argument('--client-secret', required=True, help='The secret key to use with your login provider.')
    subparser.add_argument('--redirect-uri', required=True, help='The redirect URI configured with your auth provider.')
    subparser.add_argument('--provider-uri', required=False, help='The URI for a generic openid connect provider. This option is only used for generic openid providers.')
    subparser.add_argument('--provider-port', required=False, help='The port your provider listens on for their api. This option is only used for generic openid providers.')
    subparser.add_argument('--provider-path', required=False, help='The path portion of your provider\'s uri. This option is only used for generic openid providers.')
    __add_common_args(subparser)
    subparser.set_defaults(func=player_identity.add_login_provider)

    subparser = subparsers.add_parser('update-login-provider')
    subparser.add_argument('--provider-name', required=True, help='The name of the provider e.g. amazon, google, facebook. If you are using a generic openid provider its unique name must be specified here and the provider-path, provider-uri, and/or provider-port options can be provided.')
    subparser.add_argument('--app-id', required=False, help='The application id from your login provider (this is usually different from your client id though not always). This is the value that would go into your cognito-identity configuration.')
    subparser.add_argument('--client-id', required=False, help='The unique application client id for the login provider.')
    subparser.add_argument('--client-secret', required=False, help='The secret key to use with your login provider.')
    subparser.add_argument('--redirect-uri', required=False, help='The redirect URI configured with your auth provider.')
    subparser.add_argument('--provider-uri', required=False, help='The URI for a generic openid connect provider. This option is only use for generic openid providers.')
    subparser.add_argument('--provider-port', required=False, help='The port your provider listens on for their api. This option is only used for generic openid providers.')
    subparser.add_argument('--provider-path', required=False, help='The path portion of your provider\'s uri. This option is only used for generic openid providers.')
    __add_common_args(subparser)
    subparser.set_defaults(func=player_identity.update_login_provider)

    subparser = subparsers.add_parser('remove-login-provider')
    subparser.add_argument('--provider-name', required=True, help='The name of the provider e.g. amazon, google, facebook; or the unique name of a generic openid provider.')
    __add_common_args(subparser)
    subparser.set_defaults(func=player_identity.remove_login_provider)

    subparser = subparsers.add_parser('list-profiles')
    subparser.add_argument('--profile', required=False, help='The name of the profile to list. All profies are listed by default.')
    subparser.set_defaults(func=profile.list)

    subparser = subparsers.add_parser('add-profile')
    subparser.add_argument('--aws-access-key', required=True, help='The AWS access key associated with the added profile.')
    subparser.add_argument('--aws-secret-key', required=True, help='The AWS secret key associated with the added profile.')
    subparser.add_argument('--profile', required=True, help='The name of the AWS profile to add.')
    subparser.add_argument('--make-default', required=False, action='store_true')
    subparser.set_defaults(func=profile.add)

    subparser = subparsers.add_parser('update-profile')
    subparser.add_argument('--aws-access-key', required=False, help='The AWS access key associated with the updated profile. Default is to not change the AWS access key associated with the profile.')
    subparser.add_argument('--aws-secret-key', required=False, help='The AWS secret key associated with the added profile. Default is to not change the AWS secret key associated with the profile.')
    subparser.add_argument('--profile', required=True, help='The name of the AWS profile to add.')
    subparser.set_defaults(func=profile.update)

    subparser = subparsers.add_parser('remove-profile')
    subparser.add_argument('--profile', required=True, help='The name of the AWS profile to add.')
    subparser.set_defaults(func=profile.remove)

    subparser = subparsers.add_parser('rename-profile')
    subparser.add_argument('--old-name', required=True, help='The name of the AWS profile to change.')
    subparser.add_argument('--new-name', required=True, help='The new name of the AWS profile.')
    subparser.set_defaults(func=profile.rename)

    subparser = subparsers.add_parser('default-profile')
    group = subparser.add_mutually_exclusive_group()
    group.add_argument('--set', metavar='PROFILE', help='Set the default profile to the provided deployment name. ')
    group.add_argument('--clear', action='store_true', help='Clear the default. ')
    group.add_argument('--show', action='store_true', help='Show the defaults. ')
    __add_common_args(subparser)
    subparser.set_defaults(func=profile.default)

    subparser = subparsers.add_parser('list-parameters')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='Shows only parameters for the specified deployment. By default parameters for all deployments are shown.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP', required=False, help='Shows only parameters for the specified resource group. By default parameters for all resource groups are shown.')
    subparser.add_argument('--parameter', metavar='PARAMETER', required=False, help='Shows only the specified parameter. By default all parameters are shown.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.list_parameters)

    subparser = subparsers.add_parser('set-parameter')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=True, help='Sets the parameter value for the specified deployment. Use * to set the parameter for all deployments.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP', required=True, help='Sets the parameter value for the specified resource group. Use * to set the parameter for all resource groups.')
    subparser.add_argument('--parameter', metavar='PARAMETER', required=True, help='The name of the parameter to set.')
    subparser.add_argument('--value', metavar='VALUE', required=True, help='The parameter\'s value.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.set_parameter)

    subparser = subparsers.add_parser('clear-parameter')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='Clears the parameter value for the specified deployment. Use * to clear a parameter that has been set for all deployments. If ommited, the parameter will be cleared for deployments.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP', required=False, help='Clears the parameter value for the specified resource group. Use * to set the parameter for all resource groups. If ommitted the parameter will be cleared for all resource groups.')
    subparser.add_argument('--parameter', metavar='PARAMETER', required=True, help='The name of the parameter to clear.')
    subparser.add_argument('--confirm-clear', required=False, action='store_true', help='Confirms that you want to clear the parameter values. Required only if the --deployment or --resource-group option is omitted. By default you are prompted to confirm the change.')
    __add_common_args(subparser)
    subparser.set_defaults(func=resource_group.clear_parameter)

    subparser = subparsers.add_parser('update-deployment-access-stack')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='The name of the deployment whos access stack is updated. If omitted, the default deployment is updated. Use * to update all deployments.')
    subparser.add_argument('--confirm-resource-deletion', '-D', action='store_true', help='Confirms that you know this command will permanently delete resources.')
    subparser.add_argument('--confirm-security-change', '-S', action='store_true', help='Confirms that you know this command will make security related changes.')
    subparser.add_argument('--confirm-aws-usage', '-C', action='store_true', help='Confirms that you know this command will create AWS resources for which you may be charged and that it may perform actions that can affect permissions in your AWS account.')
    __add_common_args(subparser)
    subparser.set_defaults(func=deployment.update_access_stack)

    subparser = subparsers.add_parser('get-function-log')
    subparser.add_argument('--log-stream-name', metavar='LOG-STREAM', required=False, help='The log stream name, or part of a log stream mane. If omitted, the most recent log stream will be shown.')
    subparser.add_argument('--function', metavar='FUNCTION', required=True, help='The logical name of a lambda function resource.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT', required=False, help='The name of a deployment. If given then --resource-group must also be given. If ommitted, then the function must exist in the project stack.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP', required=False, help='The name of a resource group. If given then --deployment must also be given.')
    __add_common_args(subparser)
    subparser.set_defaults(func=function.get_function_log)


def __add_built_in_commands(context, subparsers):
    '''Adds the built in sub-commands'''

    # importer commands
    subparser = subparsers.add_parser('resource-importer', help='List and import copies of existing AWS resources to your resource groups')
    __add_resource_importer_commands(subparser)

    # project stack commands
    subparser = subparsers.add_parser('project', aliases=['p'], help='Perform operations on the project stack')
    __add_project_stack_commands(subparser)

    # resource group commands
    subparser = subparsers.add_parser('resource-group', aliases=['r'], help='Perform resource-group level operations')
    __add_resource_group_commands(subparser)

    # deployment commands
    subparser = subparsers.add_parser('deployment', aliases=['d'], help='Create, update, and delete deployments, and to configure deployment defaults')
    __add_deployment_commands(subparser)

    # profile commands
    subparser = subparsers.add_parser('profile', help='Manage AWS profiles')
    __add_profile_commands(subparser)

    # mappings commands
    subparser = subparsers.add_parser('mappings', help='Show or update the logical to physical resource name mappings')
    __add_mappings_commands(subparser)

    # login provider commands
    subparser = subparsers.add_parser('login-provider', help='Manage login providers to your Player Access template, so that you can connect your application to cognito-identity.')
    __add_login_provider_commands(subparser)

    # parameter commands
    subparser = subparsers.add_parser('parameter', help='Add, set, and clear parameter values used when updated resource group stacks')
    __add_parameter_commands(subparser)

    # function commands
    subparser = subparsers.add_parser('function', aliases=['f'], help='Upload lambda function code, or fetch logs from an AWS Lambda function')
    __add_function_commands(subparser)

    # log finder commands
    subparser = subparsers.add_parser('logfind', aliases=[
                                      'lf'], help='Find cloudwatch logs associated with a specific request')
    __add_log_finder_commands(subparser)

    # backup commands
    subparser = subparsers.add_parser('backup', help='Backup dynamodb and s3 resources')
    __add_backup_commands(subparser)

    # restore s3 bucket
    subparser = subparsers.add_parser('restores3', help="Transfer the contents from one s3 bucket to another, to be used with lmbr_aws backup")
    subparser.add_argument('--deployment', '-d', required=False, metavar='DEPLOYMENT',
                                    help='The deployment to search logs in. If not specified the default deployment is used')
    subparser.add_argument('--resource', '-r', required=True,
                                  help='The resource to backup, in <group>.<resource> format')
    subparser.add_argument(
        '--backup-name', '-b', required=False, help='The name of the backup.')
    subparser.set_defaults(func=backup.restore_bucket)

    # security commands
    security.add_cli_commands(subparsers, __add_common_args)

    # gem commands
    gem.add_gem_cli_commands(context, subparsers, __add_common_args)

    # deprecated commands
    __add_deprecated_commands(context, subparsers)


def __add_hook_module_commands(context, subparsers):

    # Deprecated in 1.9. TODO: remove.
    context.hooks.call_module_handlers('cli-plugin-code/resource_commands.py','add_parser_commands',
        args=[subparsers, __add_common_args],
        deprecated=True
    )

    context.hooks.call_module_handlers('resource-manager-code/command.py','add_cli_commands',
        kwargs={
            'subparsers': subparsers,
            'add_common_args': __add_common_args
        }
    )


class AliasedSubParsersAction(argparse._SubParsersAction):

    class _AliasedPseudoAction(argparse.Action):
        def __init__(self, name, aliases, help):
            dest = name
            if aliases:
                dest += ' (%s)' % ','.join(aliases)
            sup = super(AliasedSubParsersAction._AliasedPseudoAction, self)
            sup.__init__(option_strings=[], dest=dest, help=help)

    def add_parser(self, name, **kwargs):
        if 'aliases' in kwargs:
            aliases = kwargs['aliases']
            del kwargs['aliases']
        else:
            aliases = []

        parser = super(AliasedSubParsersAction, self).add_parser(name, **kwargs)

        # Make the aliases work.
        for alias in aliases:
            self._name_parser_map[alias] = parser
        # Make the help text reflect them, first removing old help entry.
        if 'help' in kwargs:
            help = kwargs.pop('help')
            self._choices_actions.pop()
            pseudo_action = self._AliasedPseudoAction(name, aliases, help)
            self._choices_actions.append(pseudo_action)

        return parser

if __name__ == "__main__":
    sys.exit(main())
