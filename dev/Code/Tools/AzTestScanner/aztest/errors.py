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
from collections import defaultdict


class AzTestError(Exception):
    """Base class for exceptions in the AzTestScanner."""
    pass


class InvalidUseError(AzTestError):
    """Exception to indicate invalid use of a class or function."""
    pass


class RunnerReturnCodes:
    """ Defines return and error codes from running the test scanner.
    """

    def __init__(self):
        raise InvalidUseError("RunnerReturnCodes should not be instantiated!")

    TESTS_SUCCEEDED = 0
    TESTS_FAILED = 1
    
    INCORRECT_USAGE = 101
    FAILED_TO_LOAD_LIBRARY = 102
    SYMBOL_NOT_FOUND = 103
    MODULE_SKIPPED = 104 # this is not an error condition but isn't the same as tests running and succeeding.

    MODULE_TIMEOUT = 106

    UNEXPECTED_EXCEPTION = 900
    NTSTATUS_BREAKPOINT = -2147483645L
    ACCESS_VIOLATION = -1073741819L

    CODE_TO_STRING_DICT = defaultdict(lambda: "Unknown return code", **{
        TESTS_SUCCEEDED: None,
        TESTS_FAILED: "Tests Failed",

        INCORRECT_USAGE: "Incorrect usage of test runner",
        FAILED_TO_LOAD_LIBRARY: "Failed to load library",
        SYMBOL_NOT_FOUND: "Symbol not found",
        MODULE_SKIPPED: "Skipped (Not an error)",

        MODULE_TIMEOUT: "Module timed out.",

        UNEXPECTED_EXCEPTION: "Unexpected Exception while scanning module",
        NTSTATUS_BREAKPOINT: "NTSTATUS: STATUS_BREAKPOINT",
        ACCESS_VIOLATION: "ACCESS_VIOLATION"
    })

    @staticmethod
    def to_string(return_code):
        return RunnerReturnCodes.CODE_TO_STRING_DICT[return_code]
