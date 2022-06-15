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

import resource_manager.cli
import wc_service_api


def add_cli_commands(hook, subparsers, add_common_args, **kwargs):
    subparser = subparsers.add_parser("web-communicator", help="Commands to manage the CloudGemWebCommunicator gem")
    subparser.register('action', 'parsers', resource_manager.cli.AliasedSubParsersAction)
    web_communicator_subparsers = subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = web_communicator_subparsers.add_parser('list-channels', help='List all the existing channels')
    add_common_args(subparser)
    subparser.set_defaults(func=wc_service_api.command_list_all_channels)

    subparser = web_communicator_subparsers.add_parser('list-users', help='List all the existing users')
    add_common_args(subparser)
    subparser.set_defaults(func=wc_service_api.command_list_all_users)

    subparser = web_communicator_subparsers.add_parser('register-client', help='Register the client')
    subparser.add_argument('--path', type=str, required=False,
                           help='The output folder to target. Defaults to current directory')
    subparser.add_argument('--type', type=str, required=True,
                           choices=wc_service_api.Constants.CLIENT_REGISTRATION_TYPES,
                           help='The type of the registration.')
    add_common_args(subparser)
    subparser.set_defaults(func=wc_service_api.command_set_client_registration)

    subparser = web_communicator_subparsers.add_parser('send-message', help='Send a message on a channel')
    subparser.add_argument('--channel-name', type=str, required=True, help='Channel name to send message on')
    subparser.add_argument('--client-id', type=str,
                           help='Client id to send message to (the cognito id of the player to target).'
                                'If not provided message is broadcast to all clients on channel')
    subparser.add_argument('--message', type=str,
                           help=f'The message to broadcast. Max {wc_service_api.Constants.MESSAGE_SEND_MAX_LENGTH} chars')
    add_common_args(subparser)
    subparser.set_defaults(func=wc_service_api.command_send_message)

    subparser = web_communicator_subparsers.add_parser('set-user-status', help='Set the status of a user')
    subparser.add_argument('--client-id', type=str, required=True, help='The client id to set the status for')
    subparser.add_argument('--status', type=str, required=True, choices=wc_service_api.Constants.USER_STATUS,
                           help='Status to set')
    add_common_args(subparser)
    subparser.set_defaults(func=wc_service_api.command_set_user_status)

    subparser = web_communicator_subparsers.add_parser('show-logs', help='Show recent log events for ServiceLambda')
    subparser.add_argument('--minutes', type=int, default=10, required=False,
                           help='How far back from now to attempt to display. Default is 10 minutes')
    add_common_args(subparser)
    subparser.set_defaults(func=wc_service_api.command_show_log_events)
