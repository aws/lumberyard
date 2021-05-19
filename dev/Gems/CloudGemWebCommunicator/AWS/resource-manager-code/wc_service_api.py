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
import ast
import os
import json

import boto3

import cgf_service_client
import resource_manager.util
from resource_manager.service import Service
from resource_manager.service import ServiceLogs


class Constants:
    USER_STATUS = ['REGISTERED', 'BANNED']  # Valid user states
    CLIENT_REGISTRATION_TYPES = ['OPENSSL', 'WEBSOCKET']  # Valid client types
    MESSAGE_SEND_MAX_LENGTH = 1024  # Max length of a message to send, set much lower that MQTT limits
    DEVICE_INFO_JSON = 'deviceinfo.json'  # Must match name from CloudGemWebCommunicatorComponent
    DEVICE_PEM = 'webcommunicatordevice.pem'  # Must match name from CloudGemWebCommunicatorComponent
    KEY_PEM = 'webcommunicatorkey.pem'  # Must match name from CloudGemWebCommunicatorComponent


def __get_default_resource_group() -> str:
    """Get the resource group name for CloudGemWebCommunicator"""
    return 'CloudGemWebCommunicator'


def __encode_query_parameter(value: str) -> str:
    return urllib.parse.quote_plus(value)


def __get_service_api_client(context, args) -> cgf_service_client.Path:
    """
    Get a service client based on the WebCommunicator deployment stack
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
        raise RuntimeError('Unable find to deployment stack for WebCommunicator')
    return stack_id


def command_list_all_channels(context, args) -> None:
    """
    List all channels via portal/channel

    Note: portal/channels returns a list and not a dict style response
    so it will get re-packed by Service client into a generic {'Result': value} dict
    """
    client = __get_service_api_client(context, args)
    response = client.navigate('portal', 'channel').GET()
    channels = response.DATA.get('Result', [])
    print(f'Found {len(channels)} channels')
    for channel in channels:
        pprint.pprint(channel)


def command_list_all_users(context, args) -> None:
    """
    List all users via portal/users
    """
    client = __get_service_api_client(context, args)
    response = client.navigate('portal', 'users').GET()
    users = response.DATA.get('UserInfoList', [])
    print(f'Found {len(users)} users')
    for user in users:
        pprint.pprint(user)


def command_set_user_status(context, args) -> None:
    """
    Set the user status to one of USER_STATUS
    """
    body = {
        "ClientID": args.client_id,
        "RegistrationStatus": args.status,
        "CGPUser": True
    }
    client = __get_service_api_client(context, args)
    response = client.navigate('portal', 'users').POST(body=body)
    pprint.pprint(response.DATA)


def __generate_device_info_json(data: dict, path: str) -> str:
    # Keys are documented in:
    # https://docs.aws.amazon.com/lumberyard/latest/userguide/cloud-canvas-cloud-gem-web-communicator-service-api.html#cloud-canvas-cloud-gem-web-communicator-service-api-get-clientregistrationregistration-type
    info = {
        "endpoint": data.get('Endpoint', ''),
        "endpointPort": data.get('EndpointPort', 443),
        "connectionType": data.get('ConnectionType', '')
    }
    path = os.path.join(path, Constants.DEVICE_INFO_JSON)
    with open(path, 'w') as info_file:
        json.dump(info, info_file)
    return path


def __generate_key_pem(data: dict, path: str) -> str:
    _pem = data.get('PrivateKey', '').split(sep='\n')
    path = os.path.join(path, Constants.KEY_PEM)
    with open(path, 'w') as device_file:
        device_file.writelines(s + '\n' for s in _pem)
    return path


def __generate_device_pem(data: dict, path: str) -> str:
    _pem = data.get('DeviceCert', '').split(sep='\n')
    path = os.path.join(path, Constants.DEVICE_PEM)
    with open(path, 'w') as device_file:
        device_file.writelines(s + '\n' for s in _pem)
    return path


def command_set_client_registration(context, args) -> None:
    """
    Register the client as one of CLIENT_REGISTRATION_TYPES
    Note: Only OPENSSL is supported at this time
    """
    path = os.getcwd()
    if args.type == 'WEBSOCKET':
        raise RuntimeError('Only OPENSSL registrations are currently supported')

    if args.path is not None:
        if not os.path.isdir(args.path):
            raise RuntimeError(f'{args.path} is not a valid directory')
        path = args.path

    client = __get_service_api_client(context, args)
    response = client.navigate('portal', 'registration', args.type).GET()

    # Will print the DeviceCert and Private keys which need to be saved to
    # files and reformatted back into valid PEM files
    data = response.DATA
    pprint.pprint(data)

    result = data.get('Result', 'DENIED')
    if result == 'DENIED':
        raise RuntimeError('Unable to register client')
    else:
        try:
            print('Writing registration files:')
            print(__generate_device_info_json(data, path))
            print(__generate_key_pem(data, path))
            print(__generate_device_pem(data, path))
        except IOError as ioe:
            print(f"[ERROR] Unable to create registration files. Failed with {ioe}")


def command_send_message(context, args) -> None:
    """Sends a message to named channel"""
    # Truncate message to some current limit
    message = (args.message[:Constants.MESSAGE_SEND_MAX_LENGTH] + '..') if len(
        args.message) > Constants.MESSAGE_SEND_MAX_LENGTH else args.message

    client = __get_service_api_client(context, args)
    # Broadcast the message on a channel
    _body = {
        "channel": args.channel_name,
        "message": message
    }

    if args.client_id is None:
        print("--> Broadcasting message")
        response = client.navigate('cloud', 'send', 'broadcast').POST(body=_body)
    else:
        response = client.navigate('cloud', 'send', 'send', args.client_id).POST(body=_body)

    # Should get a status back, if it fails something went wrong
    status = response.DATA.get('status', None)
    if status is None:
        print('[ERROR] No response from service. Check logs')
    else:
        status = ast.literal_eval(status)
        metadata = status.get('ResponseMetadata', None)
        status_code = metadata.get('HTTPStatusCode', None)

        if status_code == 200:
            print('Message sent')
        else:
            print(f'Message failed to send: status code: {status_code}')
        pprint.pprint(status)


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
