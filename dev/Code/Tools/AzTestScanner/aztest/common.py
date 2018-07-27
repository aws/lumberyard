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
from datetime import datetime
import os
import sys
from aztest.errors import InvalidUseError
from collections import namedtuple

MODULE_UNIT_EXPORT_SYMBOL = "AzRunUnitTests"
MODULE_INTEG_EXPORT_SYMBOL = "AzRunIntegTests"
EXECUTABLE_EXPORT_SYMBOL = "ContainsAzUnitTestMain"
EXECUTABLE_UNITTEST_ARG = "--unittest"
EXECUTABLE_RUNNER_ARG = "--loadunittests"
DEFAULT_OUTPUT_PATH = "TestResults"

ScanResult = namedtuple("ScanResult", ("path", "return_code", "xml_path", "error_msg"))


class SubprocessTimeoutException(Exception):
    pass


class ModuleType:
    EXECUTABLE = 1
    LIBRARY = 2

    def __init__(self):
        pass


def clean_timestamp(timestamp):
    """Cleans timestamp by replacing colons, spaces, and periods with underscores for file creation.

    :param timestamp: the timestamp to clean
    :return: cleaned timestamp
    :rtype: str
    """
    if timestamp is None:
        return ""
    if not isinstance(timestamp, str):
        raise InvalidUseError("Timestamp must be a string")
    timestamp = timestamp.replace(':', '_')
    timestamp = timestamp.replace('.', '_')
    timestamp = timestamp.replace(' ', '_')
    return timestamp


def get_output_dir(timestamp, prefix_dir):
    return os.path.join(prefix_dir, clean_timestamp(timestamp.isoformat()) if timestamp else "NO_TIMESTAMP")


def create_output_directory(output_path, no_timestamp):
    timestamp = datetime.now() if not no_timestamp else None
    output_dir = get_output_dir(timestamp, os.path.abspath(output_path))
    if os.path.exists(output_dir):
        try:
            # remove the files in the directory rather than deleting the directory itself
            for root, dirs, files in os.walk(output_dir):
                for name in files:
                    os.remove(os.path.join(root, name))
        except Exception:
            # print error message to console and exit
            print "ERROR! Could not delete old report files. Please close any open files in {} and try again" \
                .format(output_dir)
            sys.exit(1)
    else:
        os.makedirs(output_dir)

    return output_dir


def get_module_name_from_xml_filename(xml_file):
    filename = os.path.basename(xml_file)
    base_filename, ext = os.path.splitext(filename)
    return base_filename.replace('test_results_', '')


def create_xml_output_filename(path_to_file, output_dir=""):
    if not path_to_file:
        raise InvalidUseError("XML output file requires path to test module")
    filename = os.path.basename(path_to_file)
    base_filename, ext = os.path.splitext(filename)
    return os.path.abspath(os.path.join(output_dir, 'test_results_' + base_filename + '.xml'))


def to_list(obj):
    """Generator that converts given object to a list if possible and yields each value in it.

    This function specifically handles strings, lists, sets, and tuples. All other objects are treated as a single value
    of a list, including dictionaries and other containers.

    :param obj: the object to convert
    :return: a list of the given value(s)
    :rtype: list
    """
    if obj is None:
        return
    elif isinstance(obj, str):
        yield obj
    elif isinstance(obj, list) or isinstance(obj, set) or isinstance(obj, tuple):
        for x in obj:
            yield x
    else:
        yield obj
