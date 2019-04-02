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

from AzClassHeader import AzClassHeader_Driver
from AzClassInline import AzClassInline_Driver
from AzComponentCpp import AzComponentCPP_Driver
from AzReflectionCpp import AzReflectionCPP_Driver
from AzEBusInline import AZEBusInline_Driver
from AzEBusHeader import AzEBusHeader_Driver
from AzEBusCpp import AzEBusCPP_Driver

# Factory function - called from launcher
def create_drivers(env):
    return [AZEBusInline_Driver(env), AzEBusHeader_Driver(env), AzEBusCPP_Driver(env), AzClassHeader_Driver(env), AzClassInline_Driver(env), AzComponentCPP_Driver(env), AzReflectionCPP_Driver(env)]
