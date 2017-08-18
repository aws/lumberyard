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
import logging as lg
import os
import re
from aztest.common import to_list
from aztest.errors import AzTestError
logger = lg.getLogger(__name__)

DEFAULT_WHITELIST_FILE = "lmbr_test_whitelist.txt"
DEFAULT_BLACKLIST_FILE = "lmbr_test_blacklist.txt"


def get_default_whitelist():
    """Returns the default whitelist file path if it exists, otherwise empty string

    :return: whitelist path or empty string
    :rtype: str
    """
    if os.path.exists(os.path.abspath(DEFAULT_WHITELIST_FILE)):
        return os.path.abspath(DEFAULT_WHITELIST_FILE)
    else:
        return ""


def get_default_blacklist():
    """Returns the default blacklist file path if it exists, otherwise empty string

    :return: blacklist path or empty string
    :rtype: str
    """
    if os.path.exists(os.path.abspath(DEFAULT_BLACKLIST_FILE)):
        return os.path.abspath(DEFAULT_BLACKLIST_FILE)
    else:
        return ""


class FileApprover(object):
    """Class for compiling and validating the set of files that are allowed to be tested"""
    whitelist = None
    blacklist = None

    def __init__(self, whitelist_files=None, blacklist_files=None):
        self.make_whitelist(whitelist_files)
        self.make_blacklist(blacklist_files)

    def make_whitelist(self, whitelist_files):
        """Make the whitelist from a file or list of files

        It is assumed that if no whitelist is provided, then we will scan everything not in a blacklist.

        :param whitelist_files: path to a whitelist file or list of paths
        """
        if whitelist_files:
            for whitelist_file in to_list(whitelist_files):
                self.add_whitelist(whitelist_file)
        else:
            self.whitelist = None

    def make_blacklist(self, blacklist_files):
        """Make the blacklist from a file or list of files

        It is assumed that is no blacklist is provided, then we will scan everything in a directory or what is
        whitelisted.

        :param blacklist_files: path to a blacklist file or list of paths
        """
        if blacklist_files:
            for blacklist_file in to_list(blacklist_files):
                self.add_blacklist(blacklist_file)
        else:
            self.blacklist = None

    def add_whitelist(self, whitelist_file):
        """Add the file of patterns to the whitelist

        :param whitelist_file: path to a whitelist file
        """
        if whitelist_file:
            patterns = self.get_patterns_from_file(whitelist_file)
            if patterns:
                self.whitelist = patterns | (self.whitelist or set())

    def add_blacklist(self, blacklist_file):
        """Add the file of patterns to the blacklist

        :param blacklist_file: path to a blacklist file
        """
        if blacklist_file:
            patterns = self.get_patterns_from_file(blacklist_file)
            if patterns:
                self.blacklist = patterns | (self.blacklist or set())

    def add_whitelist_patterns(self, patterns):
        """Add patterns to the whitelist

        :param patterns: regular expression pattern or list of patterns to add to whitelist
        """
        self.whitelist = set(to_list(patterns)) | (self.whitelist or set())

    def add_blacklist_patterns(self, patterns):
        """Add patterns to the blacklist

        :param patterns: regular expression pattern or list of patterns to add to blacklist
        """
        self.blacklist = set(to_list(patterns)) | (self.blacklist or set())

    def is_approved(self, file_name):
        """Checks to see if file_name is in the whitelist and not the blacklist

        :param file_name: name or path of the file to check
        :return: true if file_name in whitelist or whitelist is None and not in blacklist or blacklist is None,
        else false
        """
        return self.is_whitelisted(file_name) and not self.is_blacklisted(file_name)

    def is_whitelisted(self, file_name):
        """Checks to see if file_name is in the whitelist

        :param file_name: name or path of the file to check
        :return: true if file_name in whitelist or whitelist is None, else false
        """
        return True if not self.whitelist else self.is_in_list(file_name, self.whitelist, 'whitelist')

    def is_blacklisted(self, file_name):
        """Checks to see if file_name is in the blacklist

        :param file_name: name or path of the file to check
        :return: true if file_name not in blacklist or blacklist is None, else false
        """
        return False if not self.blacklist else self.is_in_list(file_name, self.blacklist, 'blacklist')

    @staticmethod
    def get_patterns_from_file(pattern_file):
        """Returns set of patterns from pattern_file if pattern_file is valid, otherwise returns None

        :param pattern_file: path of the whitelist or blacklist file to get patterns from
        :return: the set of patterns from the file, or None if pattern_file is invalid
        :rtype: set
        """
        if not pattern_file or not os.path.exists(pattern_file):
            raise AzTestError("Invalid module/pattern file path: {}".format(pattern_file))
        logger.info("Using module/pattern file: {}".format(pattern_file))
        with open(pattern_file, 'r') as f:
            patterns = set((line.strip() for line in f))
            return patterns

    @staticmethod
    def is_in_list(file_name, patterns, list_name='CMD'):
        """Checks file_name against the set of patterns using regex

        For the curious, we do not compile the patterns ahead of time because the 're' module uses an internal cache
        that stores compiled patterns, and it is large enough (100) that we shouldn't need to worry about patterns being
        recompiled during a scan.

        :param file_name: name or path of the file to check
        :param patterns: the set of patterns to use as regular expressions
        :param list_name: the name of the list being checked (if applicable, defaults to CMD)
        :return: true if there is a match, otherwise false
        """
        if not file_name or not patterns:
            return False
        logger.debug("Checking file: {}".format(file_name))
        for pattern in patterns:
            full_pattern = r"^.*[/\\]{}(\.dll|\.exe|\.dylib)?$".format(pattern)
            match = re.match(full_pattern, file_name, re.IGNORECASE)
            if match:
                logger.debug("Found file {} in {} patterns.".format(file_name, list_name))
                return True
        return False
