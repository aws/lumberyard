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

#ifndef CRYINCLUDE_CRYSYSTEM_STATOSCOPETEXTURESTREAMINGINTERVALGROUP_H
#define CRYINCLUDE_CRYSYSTEM_STATOSCOPETEXTURESTREAMINGINTERVALGROUP_H
#pragma once


#include "Statoscope.h"

#if ENABLE_STATOSCOPE

class CStatoscopeTextureStreamingIntervalGroup
    : public CStatoscopeIntervalGroup
    , public ITextureStreamListener
{
public:
    CStatoscopeTextureStreamingIntervalGroup();

    void Enable_Impl();
    void Disable_Impl();

public: // ITextureStreamListener Members
    virtual void OnCreatedStreamedTexture(void* pHandle, const char* name, int nMips, int nMinMipAvailable);
    virtual void OnDestroyedStreamedTexture(void* pHandle);
    virtual void OnTextureWantsMip(void* pHandle, int nMinMip);
    virtual void OnTextureHasMip(void* pHandle, int nMinMip);
    virtual void OnBegunUsingTextures(void** pHandles, size_t numHandles);
    virtual void OnEndedUsingTextures(void** pHandle, size_t numHandles);

private:
    void OnChangedTextureUse(void** pHandles, size_t numHandles, int inUse);
    void OnChangedMip(void* pHandle, int field, int mip);
};

#endif

#endif // CRYINCLUDE_CRYSYSTEM_STATOSCOPETEXTURESTREAMINGINTERVALGROUP_H
