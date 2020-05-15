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
#
pushd $(dirname "$0")
export PYTHON_DIR=../../Tools/Python

if [ -e $PYTHON_DIR ]
then
    PYTHON=$PYTHON_DIR/python3.sh
    if [ -e $PYTHON ]
    then
        LMBR_AWS_DIR=../../Tools/lmbr_aws
        $PYTHON $LMBR_AWS_DIR/cleanup.py $*
    else
        echo Could not find python3.sh in $PYTHON_DIR
    fi
else
    echo Could not find Python3 in $PYTHON_DIR
fi

popd
exit 0
