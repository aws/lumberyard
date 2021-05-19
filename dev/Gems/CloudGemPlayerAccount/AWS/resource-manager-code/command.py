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
import pa_service_api


def add_cli_commands(hook, subparsers, add_common_args, **kwargs):
    subparser = subparsers.add_parser("player-account", help="Commands to manage the CloudGemPlayerAccount gem")
    subparser.register('action', 'parsers', resource_manager.cli.AliasedSubParsersAction)
    player_account_subparsers = subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = player_account_subparsers.add_parser('add-player', help='Add a new player')
    subparser.add_argument('--username', type=str, required=True, help='The cognito username of the account to create')
    subparser.add_argument('--email', type=str, required=True, help='The email address for the player')
    subparser.add_argument('--playername', type=str, required=False, help='The name of the player in the game.')
    subparser.add_argument('--givenname', type=str, required=False, help='The players given name,')
    subparser.add_argument('--familyname', type=str, required=False, help='The players family name,')
    subparser.add_argument('--nickname', type=str, required=False, help='The players nickname')
    subparser.add_argument('--gender', type=str, required=False, choices=pa_service_api.GENDER_CHOICES, help='The players gender')
    subparser.add_argument('--locale', type=str, required=False, help='The players locale')
    add_common_args(subparser)
    subparser.set_defaults(func=pa_service_api.command_add_player)

    subparser = player_account_subparsers.add_parser('ban-player', help='Ban a player. See remove_player_ban to restore player')
    subparser.add_argument('--account-id', type=str, required=True, help='The account id to ban')
    add_common_args(subparser)
    subparser.set_defaults(func=pa_service_api.command_ban_player)

    subparser = player_account_subparsers.add_parser('confirm-player', help='Force confirm a player')
    subparser.add_argument('--username', type=str, required=True, help='The cognito username of the account to confirm')
    add_common_args(subparser)
    subparser.set_defaults(func=pa_service_api.command_confirm_player)

    subparser = player_account_subparsers.add_parser('edit-player', help='Edit a players settings')
    subparser.add_argument('--account-id', type=str, required=True, help='The account id to edit')
    subparser.add_argument('--playername', type=str, required=False, help='The name of the player in the game.')
    subparser.add_argument('--givenname', type=str, required=False, help='The players given name,')
    subparser.add_argument('--familyname', type=str, required=False, help='The players family name,')
    subparser.add_argument('--nickname', type=str, required=False, help='The players nickname,')
    subparser.add_argument('--gender', type=str, required=False, choices=pa_service_api.GENDER_CHOICES, help='The players gender')
    subparser.add_argument('--locale', type=str, required=False, help='The players locale')
    subparser.add_argument('--email', type=str, required=False, help='The email address for the player')
    add_common_args(subparser)
    subparser.set_defaults(func=pa_service_api.command_edit_player)

    subparser = player_account_subparsers.add_parser('remove-player-ban', help='Remove a player ban')
    subparser.add_argument('--account-id', type=str, required=True, help='The account id to restore')
    add_common_args(subparser)
    subparser.set_defaults(func=pa_service_api.command_remove_player_ban)

    subparser = player_account_subparsers.add_parser('reset-player-password', help='Reset a player password')
    subparser.add_argument('--username', type=str, required=True, help='The cognito username of the account to target')
    add_common_args(subparser)
    subparser.set_defaults(func=pa_service_api.command_reset_player_password)

    subparser = player_account_subparsers.add_parser('show-banned-players', help='List banned players in the Gem')
    subparser.add_argument('--page-token', type=str, required=False, default=None, help='The pagination token to get the next page.')
    add_common_args(subparser)
    subparser.set_defaults(func=pa_service_api.command_list_banned_players)

    subparser = player_account_subparsers.add_parser('show-players', help='List registered players in the Gem')
    subparser.add_argument('--filter-type', type=str, required=False, choices=pa_service_api.SEARCH_FILTER_CHOICES, help='The type of filter to apply')
    subparser.add_argument('--filter-value', type=str, required=False, help='The value for the filter as a string. '
                                                                            'For example the email address for the CognitoEmail filter.')
    subparser.add_argument('--page-token', type=str, required=False, default=None, help='The pagination token to get the next page.')
    add_common_args(subparser)
    subparser.set_defaults(func=pa_service_api.command_list_players)

    subparser = player_account_subparsers.add_parser('show-player-details', help='Show details about a player')
    subparser.add_argument('--account-id', type=str, required=True, help='The account id to show details for')
    add_common_args(subparser)
    subparser.set_defaults(func=pa_service_api.command_list_player_details)

    subparser = player_account_subparsers.add_parser('show-logs', help='Show recent log events for ServiceLambda')
    subparser.add_argument('--minutes', type=int, required=False, help='How far back from now to attempt to display. Default is 10mins')
    add_common_args(subparser)
    subparser.set_defaults(func=pa_service_api.command_show_log_events)
