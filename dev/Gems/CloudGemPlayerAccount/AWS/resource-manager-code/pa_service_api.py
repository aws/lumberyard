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
# Provides replacement functionality for PlayerAccount's CGP functions for working with
# uploaded content
import pprint
import urllib

import boto3

import cgf_service_client
import resource_manager.util
from resource_manager.service import Service
from resource_manager.service import ServiceLogs

# Valid search filters
SEARCH_FILTER_CHOICES = ['AccountId', 'PlayerName', 'CognitoIdentity', 'CognitoUsername', 'CognitoEmail']

# Note: these are the default genders supported by the gem. Needs expansion for inclusivity
GENDER_CHOICES = ['Male', 'Female', 'Both']

# Editable Cognito fields
COGNITO_ATTRIBUTES = {"email", "family_name", "gender", "given_name", "locale", "nickname"}


def __get_default_resource_group() -> str:
    """Get the resource group name for player account"""
    return 'CloudGemPlayerAccount'


def __encode_query_parameter(value: str) -> str:
    return urllib.parse.quote_plus(value)


def __get_service_api_client(context, args) -> cgf_service_client.Path:
    """
    Get a service client based on the PlayerAccount deployment stack
    """
    stack_id = __get_service_stack_id(context, args)
    service = Service(stack_id=stack_id, context=context)
    resource_region = resource_manager.util.get_region_from_arn(stack_id)
    session = boto3.Session(region_name=resource_region)
    return service.get_service_client(session=session, verbose=False)


def __get_service_stack_id(context, args) -> str:
    deployment_name = args.deployment_name or None

    resource_group_name = __get_default_resource_group()
    if deployment_name is None:
        deployment_name = context.config.default_deployment

    stack_id = context.config.get_resource_group_stack_id(deployment_name, resource_group_name, optional=True)
    if not stack_id:
        raise RuntimeError('Unable find to deployment stack for PlayerAccount')
    return stack_id


def command_list_player_details(context, args) -> None:
    """
    Search the list of players
    """
    client = __get_service_api_client(context, args)
    response = client.navigate('admin', 'accounts', args.account_id).GET()
    pprint.pprint(response.DATA)


def command_list_players(context, args) -> None:
    """
    Search the list of players based on optional filters
    """
    client = __get_service_api_client(context, args)

    # Define query params for accountSearch call
    player_name = ''
    cognito_identity_id = None
    cognito_user_name = None
    email = None
    page_token = None

    if args.filter_type is None:
        response = client.navigate('admin', 'accountSearch').GET()
    else:
        if args.filter_type == 'AccountId':
            # Special case: degrades to find player
            response = client.navigate('admin', 'accounts', __encode_query_parameter(args.filter_value)).GET()
            pprint.pprint(response.DATA)
            return
        elif args.filter_type == 'PlayerName':
            player_name = __encode_query_parameter(args.filter_value)
        elif args.filter_type == 'CognitoIdentity':
            cognito_identity_id = args.filter_value
        elif args.filter_type == 'CognitoUsername':
            cognito_user_name = args.filter_value
        elif args.filter_type == 'CognitoEmail':
            email = args.filter_value
            print(f"Search by email: {email}")

        if args.page_token is not None:
            page_token = args.page_token

        response = client.navigate('admin', 'accountSearch').GET(
            StartPlayerName=player_name,
            CognitoIdentityId=cognito_identity_id,
            CognitoUsername=cognito_user_name,
            Email=email,
            PageToken=page_token
        )

    accounts = response.DATA.get('Accounts', [])
    print(f"Found {len(accounts)} Accounts")
    for account in accounts:
        pprint.pprint(account)

    # See if we got pagination tokens. Occurs when 20 accounts are in place
    _fwd_token = response.DATA.get('next', '')
    _rwd_token = response.DATA.get('previous', '')
    if len(_fwd_token):
        print(f"Page Token: {_fwd_token}")
    elif len(_rwd_token):
        print(f"Page Token: {_rwd_token}")


def __set_ban_status(context, args, banned: bool):
    client = __get_service_api_client(context, args)
    _body = {
        "AccountBlacklisted": banned
    }
    response = client.navigate('admin', 'accounts', __encode_query_parameter(args.account_id)).PUT(body=_body)
    pprint.pprint(response.DATA)


def command_ban_player(context, args) -> None:
    """
    Ban a given player
    """
    __set_ban_status(context, args, True)


def command_remove_player_ban(context, args) -> None:
    """
   Unban a given player
    """
    __set_ban_status(context, args, False)


def command_list_banned_players(context, args) -> None:
    """
    Show a filtered view of all banned players
    """
    client = __get_service_api_client(context, args)
    response = client.navigate('ban', 'list').GET()
    players = response.DATA.get('players', [])
    print(f"Found {len(players)} banned players")

    # Note: only get CognitoUserName back from the ServiceAPI currently
    for player in players:
        print(f'CognitoUsername: {player}')


def command_confirm_player(context, args) -> None:
    """
    Force confirm a player's account
    """
    client = __get_service_api_client(context, args)
    try:
        response = client.navigate('admin', 'identityProviders', 'Cognito',
                                   'users', __encode_query_parameter(args.username), 'confirmSignUp').POST(body=None)
        pprint.pprint(response.DATA)
    except cgf_service_client.error.NotFoundError as nfe:
        print(f"[Error] {args.username} not found. {str(nfe)}")


def command_reset_player_password(context, args) -> None:
    """
    Reset a player's password
    """
    client = __get_service_api_client(context, args)
    try:
        response = client.navigate('admin', 'identityProviders', 'Cognito',
                                   'users', __encode_query_parameter(args.username), 'resetUserPassword').POST(body=None)
        pprint.pprint(response.DATA)
    except cgf_service_client.error.NotFoundError as nfe:
        print(f"[Error] {args.username} not found. {str(nfe)}")


def __set_attr_if_defined(body, args, field) -> None:
    if field in args:
        body[field] = args[field]


def command_add_player(context, args) -> None:
    """
    Add a new player account
    """
    client = __get_service_api_client(context, args)
    # Casing defined by the PlayerAccount lambda and Cognito
    # see COGNITO_ATTRIBUTES in the service lambda
    _body = {
        "CognitoUsername": args.username,
        'IdentityProviders': {
            'Cognito': {
                "email": args.email
            }
        }
    }
    if args.playername:
        _body['PlayerName'] = args.playername

    # set-up optional args
    for arg in COGNITO_ATTRIBUTES:
        if arg in vars(args) and getattr(args, arg) is not None:
            _body['IdentityProviders']['Cognito'][arg] = getattr(args, arg)

    response = client.navigate('admin', 'accounts').POST(body=_body)
    print(response)


def command_edit_player(context, args) -> None:
    """
    Edit a player's attributes
    """
    client = __get_service_api_client(context, args)
    # Cannot edit CognitoUsername so don't set it up
    # Can only edit values if they are not None
    _body = {
        'IdentityProviders': {
            'Cognito': {
            }
        }
    }
    if args.playername:
        _body['PlayerName'] = args.playername

        # set-up optional args
    for arg in COGNITO_ATTRIBUTES:
        if arg in vars(args) and getattr(args, arg) is not None:
            _body['IdentityProviders']['Cognito'][arg] = getattr(args, arg)

    response = client.navigate('admin', 'accounts', __encode_query_parameter(args.account_id)).PUT(body=_body)
    print(response)


def command_show_log_events(context, args) -> None:
    stack_id = __get_service_stack_id(context, args)
    service = ServiceLogs(stack_id=stack_id, context=context)

    # change default look back period if set
    if args.minutes:
        service.period = args.minutes

    resource_region = resource_manager.util.get_region_from_arn(stack_id)
    session = boto3.Session(region_name=resource_region)
    logs = service.get_logs(session=session)

    if len(logs) == 0:
        print('No logs found')

    for log in logs:
        print(log)
