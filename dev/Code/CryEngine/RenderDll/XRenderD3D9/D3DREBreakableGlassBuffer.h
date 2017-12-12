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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_D3DREBREAKABLEGLASSBUFFER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_D3DREBREAKABLEGLASSBUFFER_H
#pragma once

#include "CREBreakableGlassConfig.h"

//==================================================================================================
// Name: CREBreakableGlassBuffer
// Desc: Breakable glass geometry buffer management
// Author: Chris Bunner
//==================================================================================================
class CREBreakableGlassBuffer
{
public:
    CREBreakableGlassBuffer();
    ~CREBreakableGlassBuffer();

    // Geometry buffer types
    enum EBufferType
    {
        EBufferType_Plane = 0,
        EBufferType_Crack,
        EBufferType_Frag,
        EBufferType_Num
    };

    // Singleton access from Render Thread
    static CREBreakableGlassBuffer& RT_Instance();
    static void RT_ReleaseInstance();

    // Buffer allocation
    uint32  RT_AcquireBuffer(const bool isFrag);
    bool        RT_IsBufferValid(const uint32 id, const EBufferType buffType) const;
    bool        RT_ClearBuffer(const uint32 id, const EBufferType buffType);

    // Buffer updates
    bool        RT_UpdateVertexBuffer(const uint32 id, const EBufferType buffType, SVF_P3F_C4B_T2F* pVertData, const uint32 vertCount);
    bool        RT_UpdateIndexBuffer(const uint32 id, const EBufferType buffType, uint16* pIndData, const uint32 indCount);
    bool        RT_UpdateTangentBuffer(const uint32 id, const EBufferType buffType, SPipTangents* pTanData, const uint32 tanCount);

    // Buffer drawing
    bool        RT_DrawBuffer(const uint32 id, const EBufferType buffType);

    // Buffer state
    static const uint32 NoBuffer = 0;
    static const uint32 InvalidStreamHdl = ~0u;
    static const uint32 NumBufferSlots = GLASSCFG_MAX_NUM_ACTIVE_GLASS;

private:
    // Internal allocation
    void        InitialiseBuffers();
    void        ReleaseBuffers();

    // Internal drawing
    void        DrawBuffer(const uint32 cyclicId, const EBufferType buffType, const uint32 indCount);

    // Common buffer state/data
    struct SBuffer
    {
        SBuffer()
            : pVertices(NULL)
            , pIndices(NULL)
            , tangentStreamHdl(InvalidStreamHdl)
            , lastIndCount(0)
        {
        }

        // Geometry buffers
        CVertexBuffer*  pVertices;
        CIndexBuffer*       pIndices;
        buffer_handle_t tangentStreamHdl;

        // State
        uint32                  lastIndCount;
    };

    // Buffer data
    ILINE bool IsVBufferValid(const SBuffer& buffer) const
    {
        return buffer.pVertices && buffer.pIndices && buffer.tangentStreamHdl != InvalidStreamHdl;
    }

    SBuffer m_buffer[EBufferType_Num][NumBufferSlots];
    uint32  m_nextId[EBufferType_Num];

    // Singleton instance
    static CREBreakableGlassBuffer* s_pInstance;
};

#endif // _CRE_BREAKABLE_GLASS_BUFFER_