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

#ifndef CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXPARTICLEEFFECT_H
#define CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXPARTICLEEFFECT_H
#pragma once


#include "MFXEffectBase.h"

struct SMFXParticleEntry
{
    string name;
    string userdata;
    float scale; // base scale
    float maxdist; // max distance for spawning this effect
    float minscale; // min scale (distance == 0)
    float maxscale; // max scale (distance == maxscaledist)
    float maxscaledist;
    bool  attachToTarget;

    SMFXParticleEntry()
        : scale(1.0f)
        , maxdist(0.0f)
        , minscale(0.0f)
        , maxscale(0.0f)
        , maxscaledist(0.0f)
        , attachToTarget(false)
    {
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(name);
        pSizer->AddObject(userdata);
    }
};

typedef std::vector<SMFXParticleEntry> SMFXParticleEntries;

struct SMFXParticleParams
{
    enum EDirectionType
    {
        eDT_Normal   = 0,
        eDT_Ricochet,
    };

    SMFXParticleEntries m_entries;
    EDirectionType      directionType;
};

class CMFXParticleEffect
    : public CMFXEffectBase
{
    typedef CryFixedStringT<32> TAttachmentName;

public:

    CMFXParticleEffect();
    virtual ~CMFXParticleEffect();

    //IMFXEffect
    virtual void Execute(const SMFXRunTimeEffectParams& params) override;
    virtual void LoadParamsFromXml(const XmlNodeRef& paramsNode) override;
    virtual void GetResources(SMFXResourceList& resourceList) const override;
    virtual void PreLoadAssets() override;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const override;
    //~IMFXEffect

protected:

    bool AttachToTarget(const SMFXParticleEntry& particleParams, const SMFXRunTimeEffectParams& params, IParticleEffect* pParticleEffect, const Vec3& dir, float scale);
    bool AttachToCharacter(IEntity& targetEntity, const SMFXParticleEntry& particleParams, const SMFXRunTimeEffectParams& params, const Vec3& dir, float scale);
    bool AttachToEntity(IEntity& targetEntity, const SMFXParticleEntry& particleParams, const SMFXRunTimeEffectParams& params, IParticleEffect* pParticleEffect, const Vec3& dir, float scale);

    void GetNextCharacterAttachmentName(TAttachmentName& attachmentName);

    SMFXParticleParams m_particleParams;
};

#endif // CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXPARTICLEEFFECT_H
