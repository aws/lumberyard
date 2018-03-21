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
# $Revision: #5 $

# Make an instance of LambdaSettingModule look like the module so we can use a 
# @propery to lazy load the settings.
#
# https://stackoverflow.com/questions/880530/can-modules-have-properties-the-same-way-that-objects-can

import sys

from module import LambdaSettingsModule

sys.modules[__name__] = LambdaSettingsModule()
