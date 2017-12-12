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

// Description : Render context (set of flags)

#ifndef CRYINCLUDE_CRYAISYSTEM_DEBUGDRAWCONTEXT_H
#define CRYINCLUDE_CRYAISYSTEM_DEBUGDRAWCONTEXT_H
#pragma once

#include "IAIDebugRenderer.h"
#include "Environment.h"


class CDebugDrawContext
{
    IAIDebugRenderer* m_pDebugRenderer;
    const unsigned int m_uiDepth;

public:
    CDebugDrawContext()
        : m_pDebugRenderer(gAIEnv.CVars.NetworkDebug ? gAIEnv.GetNetworkDebugRenderer() : gAIEnv.GetDebugRenderer())
        , m_uiDepth(m_pDebugRenderer->PushState())  {}

    ~CDebugDrawContext()    { unsigned int uiDepth = m_pDebugRenderer->PopState(); assert(uiDepth + 1 == m_uiDepth); }

    IAIDebugRenderer* operator->() const { return m_pDebugRenderer; }
};


#endif // CRYINCLUDE_CRYAISYSTEM_DEBUGDRAWCONTEXT_H
