#!/usr/bin/env python

"""Starts an interactive JIRA session in an ipython terminal.

Script arguments support changing the server and a persistent authentication over HTTP BASIC.
"""

try:
    import configparser
except ImportError:
    from six.moves import configparser

from six.moves import input
from six.moves.urllib.parse import parse_qsl

import argparse
from getpass import getpass
from jira import __version__
from jira import JIRA
from oauthlib.oauth1 import SIGNATURE_RSA
import os
import requests
from requests_oauthlib import OAuth1
from sys import exit

import webbrowser

CONFIG_PATH = os.path.join(
    os.path.expanduser('~'), '.jira-python', 'jirashell.ini')


def oauth_dance(server, consumer_key, key_cert_data, print_tokens=False, verify=None):
    if verify is None:
        verify = server.startswith('https')

    # step 1: get request tokens
    oauth = OAuth1(
        consumer_key, signature_method=SIGNATURE_RSA, rsa_key=key_cert_data)
    r = requests.post(
        server + '/plugins/servlet/oauth/request-token', verify=verify, auth=oauth)
    request = dict(parse_qsl(r.text))
    request_token = request['oauth_token']
    request_token_secret = request['oauth_token_secret']
    if print_tokens:
        print("Request tokens received.")
        print("    Request token:        {}".format(request_token))
        print("    Request token secret: {}".format(request_token_secret))

    # step 2: prompt user to validate
    auth_url = '{}/plugins/servlet/oauth/authorize?oauth_token={}'.format(
        server, request_token)
    if print_tokens:
        print(
            "Please visit this URL to authorize the OAuth request:\n\t{}".format(auth_url))
    else:
        webbrowser.open_new(auth_url)
        print(
            "Your browser is opening the OAuth authorization for this client session.")

    approved = input(
        'Have you authorized this program to connect on your behalf to {}? (y/n)'.format(server))

    if approved.lower() != 'y':
        exit(
            'Abandoning OAuth dance. Your partner faceplants. The audience boos. You feel shame.')

    # step 3: get access tokens for validated user
    oauth = OAuth1(consumer_key,
                   signature_method=SIGNATURE_RSA,
                   rsa_key=key_cert_data,
                   resource_owner_key=request_token,
                   resource_owner_secret=request_token_secret
                   )
    r = requests.post(
        server + '/plugins/servlet/oauth/access-token', verify=verify, auth=oauth)
    access = dict(parse_qsl(r.text))

    if print_tokens:
        print("Access tokens received.")
        print("    Access token:        {}".format(access['oauth_token']))
        print("    Access token secret: {}".format(
            access['oauth_token_secret']))

    return {
        'access_token': access['oauth_token'],
        'access_token_secret': access['oauth_token_secret'],
        'consumer_key': consumer_key,
        'key_cert': key_cert_data}


def process_config():
    if not os.path.exists(CONFIG_PATH):
        return {}, {}, {}

    parser = configparser.ConfigParser()
    try:
        parser.read(CONFIG_PATH)
    except configparser.ParsingError as err:
        print("Couldn't read config file at path: {}\n{}".format(
            CONFIG_PATH, err))
        raise

    if parser.has_section('options'):
        options = {}
        for option, value in parser.items('options'):
            if option in ("verify", "async"):
                value = parser.getboolean('options', option)
            options[option] = value
    else:
        options = {}

    if parser.has_section('basic_auth'):
        basic_auth = dict(parser.items('basic_auth'))
    else:
        basic_auth = {}

    if parser.has_section('oauth'):
        oauth = {}
        for option, value in parser.items('oauth'):
            if option in ("oauth_dance", "print_tokens"):
                value = parser.getboolean('oauth', option)
            oauth[option] = value
    else:
        oauth = {}

    return options, basic_auth, oauth


def process_command_line():
    parser = argparse.ArgumentParser(
        description='Start an interactive JIRA shell with the REST API.')
    jira_group = parser.add_argument_group('JIRA server connection options')
    jira_group.add_argument('-s', '--server',
                            help='The JIRA instance to connect to, including context path.')
    jira_group.add_argument('-r', '--rest-path',
                            help='The root path of the REST API to use.')
    jira_group.add_argument('--auth-url',
                            help='Path to URL to auth against.')
    jira_group.add_argument('-v', '--rest-api-version',
                            help='The version of the API under the specified name.')

    jira_group.add_argument('--no-verify', action='store_true',
                            help='do not verify the ssl certificate')

    basic_auth_group = parser.add_argument_group('BASIC auth options')
    basic_auth_group.add_argument('-u', '--username',
                                  help='The username to connect to this JIRA instance with.')
    basic_auth_group.add_argument('-p', '--password',
                                  help='The password associated with this user.')
    basic_auth_group.add_argument('-P', '--prompt-for-password', action='store_true',
                                  help='Prompt for the password at the command line.')

    oauth_group = parser.add_argument_group('OAuth options')
    oauth_group.add_argument('-od', '--oauth-dance', action='store_true',
                             help='Start a 3-legged OAuth authentication dance with JIRA.')
    oauth_group.add_argument('-ck', '--consumer-key',
                             help='OAuth consumer key.')
    oauth_group.add_argument('-k', '--key-cert',
                             help='Private key to sign OAuth requests with (should be the pair of the public key\
                                   configured in the JIRA application link)')
    oauth_group.add_argument('-pt', '--print-tokens', action='store_true',
                             help='Print the negotiated OAuth tokens as they are retrieved.')

    oauth_already_group = parser.add_argument_group(
        'OAuth options for already-authenticated access tokens')
    oauth_already_group.add_argument('-at', '--access-token',
                                     help='OAuth access token for the user.')
    oauth_already_group.add_argument('-ats', '--access-token-secret',
                                     help='Secret for the OAuth access token.')

    args = parser.parse_args()

    options = {}
    if args.server:
        options['server'] = args.server

    if args.rest_path:
        options['rest_path'] = args.rest_path

    if args.auth_url:
        options['auth_url'] = args.auth_url

    if args.rest_api_version:
        options['rest_api_version'] = args.rest_api_version

    options['verify'] = True
    if args.no_verify:
        options['verify'] = False

    if args.prompt_for_password:
        args.password = getpass()

    basic_auth = {}
    if args.username:
        basic_auth['username'] = args.username

    if args.password:
        basic_auth['password'] = args.password

    key_cert_data = None
    if args.key_cert:
        with open(args.key_cert, 'r') as key_cert_file:
            key_cert_data = key_cert_file.read()

    oauth = {}
    if args.oauth_dance:
        oauth = {
            'oauth_dance': True,
            'consumer_key': args.consumer_key,
            'key_cert': key_cert_data,
            'print_tokens': args.print_tokens}
    elif args.access_token and args.access_token_secret and args.consumer_key and args.key_cert:
        oauth = {
            'access_token': args.access_token,
            'oauth_dance': False,
            'access_token_secret': args.access_token_secret,
            'consumer_key': args.consumer_key,
            'key_cert': key_cert_data}

    return options, basic_auth, oauth


def get_config():
    options, basic_auth, oauth = process_config()

    cmd_options, cmd_basic_auth, cmd_oauth = process_command_line()

    options.update(cmd_options)
    basic_auth.update(cmd_basic_auth)
    oauth.update(cmd_oauth)

    return options, basic_auth, oauth


def main():
    try:
        get_ipython
    except NameError:
        pass
    else:
        exit("Running ipython inside ipython isn't supported. :(")

    options, basic_auth, oauth = get_config()

    if basic_auth:
        basic_auth = (basic_auth['username'], basic_auth['password'])

    if oauth.get('oauth_dance') is True:
        oauth = oauth_dance(
            options['server'], oauth['consumer_key'], oauth['key_cert'], oauth['print_tokens'], options['verify'])
    elif not all((oauth.get('access_token'), oauth.get('access_token_secret'),
                  oauth.get('consumer_key'), oauth.get('key_cert'))):
        oauth = None

    jira = JIRA(options=options, basic_auth=basic_auth, oauth=oauth)

    import IPython
    # The top-level `frontend` package has been deprecated since IPython 1.0.
    if IPython.version_info[0] >= 1:
        from IPython.terminal.embed import InteractiveShellEmbed
    else:
        from IPython.frontend.terminal.embed import InteractiveShellEmbed

    ipshell = InteractiveShellEmbed(
        banner1='<JIRA Shell ' + __version__ + ' (' + jira.client_info() + ')>')
    ipshell("*** JIRA shell active; client is in 'jira'."
            ' Press Ctrl-D to exit.')


if __name__ == '__main__':
    status = main()
    exit(status)
