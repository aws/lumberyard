#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
This module allows people to keep their jira server credentials outside their script, in a configuration file that is not saved in the source control.

Also, this simplifies the scripts by not having to write the same initialization code for each script.

"""
import logging
import os
import sys
try:
    import configparser
except ImportError:
    from six.moves import configparser

from jira.client import JIRA


def get_jira(profile=None, url="http://localhost:2990", username="admin", password="admin", appid=None, autofix=False, verify=True):
    """Return a JIRA object by loading the connection details from the `config.ini` file.

    :param profile: The name of the section from config.ini file that stores server config url/username/password
    :param url: URL of the Jira server
    :param username: username to use for authentication
    :param password: password to use for authentication
    :param verify: boolean indicating whether SSL certificates should be verified
    :return: JIRA -- an instance to a JIRA object.
    :raises: EnvironmentError

    Usage:

        >>> from jira.config import get_jira
        >>>
        >>> jira = get_jira(profile='jira')

    Also create a `config.ini` like this and put it in current directory, user home directory or PYTHONPATH.

    .. code-block:: none

        [jira]
        url=https://jira.atlassian.com
        # only the `url` is mandatory
        user=...
        pass=...
        appid=...
        verify=...

    """
    def findfile(path):
        """Find the file named path in the sys.path.

        Returns the full path name if found, None if not found
        """
        paths = ['.', os.path.expanduser('~')]
        paths.extend(sys.path)
        for dirname in paths:
            possible = os.path.abspath(os.path.join(dirname, path))
            if os.path.isfile(possible):
                return possible
        return None
    config = configparser.ConfigParser(defaults={'user': None, 'pass': None, 'appid': appid, 'autofix': autofix,
                                                 'verify': 'yes' if verify else 'no'})

    config_file = findfile('config.ini')
    if config_file:
        logging.debug("Found %s config file" % config_file)

    if not profile:
        if config_file:
            config.read(config_file)
            try:
                profile = config.get('general', 'default-jira-profile')
            except configparser.NoOptionError:
                pass

    if profile:
        if config_file:
            config.read(config_file)
            url = config.get(profile, 'url')
            username = config.get(profile, 'user')
            password = config.get(profile, 'pass')
            appid = config.get(profile, 'appid')
            autofix = config.get(profile, 'autofix')
            verify = config.getboolean(profile, 'verify')

        else:
            raise EnvironmentError(
                "%s was not able to locate the config.ini file in current directory, user home directory or PYTHONPATH." % __name__)

    options = JIRA.DEFAULT_OPTIONS
    options['server'] = url
    options['autofix'] = autofix
    options['appid'] = appid
    options['verify'] = verify

    return JIRA(options=options, basic_auth=(username, password))
    # self.jira.config.debug = debug
