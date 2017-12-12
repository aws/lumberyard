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

#ifndef CRYINCLUDE_CRYMOVIE_ANIMLIGHTNODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMLIGHTNODE_H

#pragma once

#include "EntityNode.h"

template <class ValueType>
class TAnimSplineTrack;
typedef TAnimSplineTrack<Vec2> C2DSplineTrack;

// Node for light animation set sequences
// Normal lights are handled by CEntityNode
class CAnimLightNode
    : public CAnimEntityNode
{
public:
    CAnimLightNode(const int id);
    virtual ~CAnimLightNode();

    virtual void Animate(SAnimContext& ec);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    virtual void CreateDefaultTracks();
    virtual void Activate(bool bActivate);

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;
    bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

    bool SetParamValue(float time, CAnimParamType param, float value);
    bool SetParamValue(float time, CAnimParamType param, const Vec3& value);

    virtual void InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType);

private:

    enum
    {
        NumNodeParams = 10
    };

    static CAnimNode::SParamInfo s_nodeParams[NumNodeParams];

private:

    bool GetValueFromTrack(CAnimParamType type, float time, float& value) const;
    bool GetValueFromTrack(CAnimParamType type, float time, Vec3& value) const;


private:

    float m_fRadius;
    float m_fDiffuseMultiplier;
    float m_fHDRDynamic;
    float m_fSpecularMultiplier;
    float m_fSpecularPercentage;
    Vec3 m_clrDiffuseColor;
    bool m_bJustActivated;
};

#endif // CRYINCLUDE_CRYMOVIE_ANIMLIGHTNODE_H
