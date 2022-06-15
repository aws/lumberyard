#!/bin/bash

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


SOURCE="${BASH_SOURCE[0]}"
# While $SOURCE is a symlink, resolve it
while [ -h "$SOURCE" ]; do
    DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
    SOURCE="$( readlink "$SOURCE" )"
    # If $SOURCE was a relative symlink (so no "/" as prefix, need to resolve it relative to the symlink base directory
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

if [[ "$OSTYPE" = *"darwin"* ]];
then
    export DYLD_FALLBACK_LIBRARY_PATH=$DIR/3.7.10/mac/Python.framework/Versions/3.7/lib/lib-dynload
    PYTHON=$DIR/3.7.10/mac/Python.framework/Versions/3.7/bin/python3
    PATH_SCRIPT=$DIR/3.7.10/extend_pythonpath.py
else
    PYTHON=$DIR/3.7.10/linux_x64/bin/python3.7
    PATH_SCRIPT=$DIR/3.7.10/extend_pythonpath.py
fi

if [ -e "$PYTHON" ]
then
    export PYTHONPATH="$($PYTHON $PATH_SCRIPT)"
    "$PYTHON" "$@"
    exit $?
else
    echo "Could not find python in $PYTHON"
    exit 1
fi

exit 1
