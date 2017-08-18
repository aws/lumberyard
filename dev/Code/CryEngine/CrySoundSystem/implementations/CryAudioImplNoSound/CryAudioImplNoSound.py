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
from makeomatic.makeomatic import*
from makeomatic.defines import*
from makeomatic.export import*
from project_utils import*
import globals
import os

TARGET = Target('CryAudioImplNoSound', TargetTypes.Module)

TARGET.flags.compile[BuildConfigs.Common].add_includes(os.path.join('%CODEROOT%','CryEngine','CrySoundSystem','implementations',TARGET.name),os.path.join('%CODEROOT%','CryEngine','CryCommon'),os.path.join('%CODEROOT%','SDKs','boost'))
		
TARGET.flags.compile[BuildConfigs.Common].add_includes(os.path.join('%CODEROOT%','CryEngine','CrySoundSystem','Common'))

VCXPROJ = 'CryAudioImplNoSound.vcxproj'

REMOVE_SOURCES_C = []
REMOVE_SOURCES_H = []
REMOVE_SOURCES_CPP = []

TARGET.precompiled_header = os.path.join('CryEngine', 'CrySoundSystem','implementations',TARGET.name, 'stdafx.h')