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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_STATOSCOPETEXTURESTREAMING_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_STATOSCOPETEXTURESTREAMING_H
#pragma once


#include "IStatoscope.h"

#if ENABLE_STATOSCOPE

#include "TextureStreamPool.h"

struct SStatoscopeTextureStreamingDG
    : public IStatoscopeDataGroup
{
    virtual void Enable();
    virtual void Disable();

    virtual SDescription GetDescription() const;
    virtual void Write(IStatoscopeFrameRecord& fr);

private:
    float GetTextureRequests();
    float GetTextureRenders();
    float GetTexturePoolUsage();
    float GetTexturePoolWanted();
    float GetTextureUpdates();
};

struct SStatoscopeTextureStreamingItemsDG
    : public IStatoscopeDataGroup
{
    virtual void Enable();
    virtual void Disable();

    virtual SDescription GetDescription() const;
    virtual uint32 PrepareToWrite();
    virtual void Write(IStatoscopeFrameRecord& fr);

private:
    std::vector<CTexture::WantedStat> m_statsTexWantedLists[2];
};

#ifndef _RELEASE

struct SStatoscopeTextureStreamingPoolDG
    : public IStatoscopeDataGroup
{
    virtual void Enable();
    virtual void Disable();

    virtual SDescription GetDescription() const;
    virtual uint32 PrepareToWrite();
    virtual void Write(IStatoscopeFrameRecord& fr);

private:
    std::vector<CTextureStreamPoolMgr::SPoolStats> m_ps;
};

#endif

#endif

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_STATOSCOPETEXTURESTREAMING_H
