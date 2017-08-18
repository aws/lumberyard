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
from logging.handlers import RotatingFileHandler


def setup_logging(filename=None, level=lg.INFO):
    import sys

    log = lg.getLogger('')
    log.setLevel(level)

    # %(asctime)s
    formatter = lg.Formatter("%(relativeCreated)s - %(levelname)s - [%(threadName)s] - %(name)s - %(message)s")

    # stdout
    ch = lg.StreamHandler(sys.stdout)
    ch.setLevel(level)
    ch.setFormatter(formatter)
    log.addHandler(ch)

    # file
    if filename:
        fh = RotatingFileHandler(filename,
                                 maxBytes=(1048576*5),
                                 backupCount=7)
        fh.setFormatter(formatter)
        log.addHandler(fh)
