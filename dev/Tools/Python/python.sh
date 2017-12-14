# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# Original file Copyright Crytek GMBH or its affiliates, used under license.
CMD_DIR=`pwd`/$(dirname "$0")
PYTHON=$CMD_DIR/2.7.13/mac/Python.framework/Versions/2.7/python.2.7.13

if [ -e $PYTHON ]
then
    $PYTHON $*
else
    echo Could not find python.2.7.13 in $CMD_DIR/2.7.13/mac/Python.framework/Versions/2.7/..
fi

exit 0
