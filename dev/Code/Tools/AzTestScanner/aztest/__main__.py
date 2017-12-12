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
import sys
from aztest import execute
logger = lg.getLogger(__name__)


def main():
    exit_code = execute()
    if exit_code != 0:
        sys.exit(exit_code)


if __name__ == "__main__":
    main()
