/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_ANIMATIONMANAGER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_ANIMATIONMANAGER_H
#pragma once

#include "GlobalAnimationHeaderAIM.h"
#include "GlobalAnimationHeaderCAF.h"
#include "IResCompiler.h"
#include "ThreadUtils.h"

#include <AzCore/std/parallel/mutex.h>

class CAnimationManager
{
public:

    void Clear()
    {
        m_arrGlobalAIM.clear();
        m_arrGlobalAnimations.clear();
    }

    bool AddAIMHeaderOnly(const GlobalAnimationHeaderAIM& header);
    bool HasAIMHeader(const GlobalAnimationHeaderAIM& header) const;
    bool AddCAFHeaderOnly(const GlobalAnimationHeaderCAF& header);
    bool HasCAFHeader(const GlobalAnimationHeaderCAF& header) const;

    bool SaveAIMImage(const char* fileName, FILETIME timeStamp, bool bigEndianFormat);
    bool SaveCAFImage(const char* fileName, FILETIME timeStamp, bool bigEndianFormat);
private:
    bool HasAIM(uint32 pathCRC) const;
    bool HasCAF(uint32 pathCRC) const;

    typedef std::vector<GlobalAnimationHeaderCAF> GlobalAnimationHeaderCAFs;
    GlobalAnimationHeaderCAFs m_arrGlobalAnimations;
    AZStd::mutex m_lockCAFs;
    
    typedef std::vector<GlobalAnimationHeaderAIM> GlobalAnimationHeaderAIMs;
    GlobalAnimationHeaderAIMs m_arrGlobalAIM;
    AZStd::mutex m_lockAIMs;
};


#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_ANIMATIONMANAGER_H
