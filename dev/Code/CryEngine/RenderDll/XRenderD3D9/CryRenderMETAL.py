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


TARGET = Target('CryRenderMETAL', TargetTypes.Module)


TARGET.flags.compile[BuildConfigs.Common].add_includes(os.path.join('%CODEROOT%','CryEngine','RenderDll'),os.path.join('%CODEROOT%','CryEngine','CryCommon'),os.path.join('%CODEROOT%','SDKs','boost'),os.path.join('%CODEROOT%','CryEngine','RenderDll','XRenderD3D9'),os.path.join('%CODEROOT%','SDKs'))

TARGET.flags.compile[BuildConfigs.Common].add_definitions('_RENDERER')

#if globals.PLATFORM.name != "mac":
#    TARGET.flags.compile[BuildConfigs.Common].add_definitions('GLEW_MX')


TARGET.flags.compile[BuildConfigs.Common].add_definitions('XRENDERGL_EXPORTS', 'DIRECT3D10','OPENGL')

TARGET.flags.compile[BuildConfigs.Common].add_includes('%LIB%INC%sdl2%','%LIB%INC%hlslcc%')

TARGET.flags.compile[BuildConfigs.Common].add_includes('%LIB%INC%opengles%')

REMOVE_SOURCES_C = ['DXMETAL/Specification%']
REMOVE_SOURCES_H = ['DXMETAL/Specification%']
REMOVE_SOURCES_CPP = ['DXPS%',
'DXMETAL/Specification%']


VCXPROJ = 'CryRenderMETAL.vcxproj'

TARGET.flags.link[BuildConfigs.Common].add_ldlibs('%LIB%LIB%hlslcc%')
if globals.PLATFORM.name == "mac":
	TARGET.sources.add_source(Source('DXGL/Implementation/AppleGPUInfoUtils.mm',SourceTypes.Source))
	TARGET.sources.add_source(Source('DXGL/Implementation/AppleGPUInfoUtils.h',SourceTypes.Header))

expand_sources(TARGET)

def post_create(target):
    if (globals.PLATFORM.name == "linux"):
        for source in target.sources.get_sources(SourceTypes.Source):
            if ('D3DHWShader.cpp' in source.path):
                source.compiler_flags.add_cppflags('-Wno-error')
                return

TARGET.precompiled_header = os.path.join('CryEngine', 'RenderDll', 'StdAfx.h')
