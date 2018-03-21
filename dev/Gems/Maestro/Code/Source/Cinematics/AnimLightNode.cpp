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

#include "Maestro_precompiled.h"
#include "AnimLightNode.h"
#include <IScriptSystem.h>
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

CAnimNode::SParamInfo CAnimLightNode::s_nodeParams[CAnimLightNode::NumNodeParams] = {
    CAnimNode::SParamInfo("Position", AnimParamType::Position, AnimValueType::Vector, IAnimNode::ESupportedParamFlags(0)),
    CAnimNode::SParamInfo("Rotation", AnimParamType::Rotation, AnimValueType::Quat, IAnimNode::ESupportedParamFlags(0)),
    CAnimNode::SParamInfo("Radius", AnimParamType::LightRadius, AnimValueType::Float, IAnimNode::ESupportedParamFlags(0)),
    CAnimNode::SParamInfo("DiffuseColor", AnimParamType::LightDiffuse, AnimValueType::RGB, IAnimNode::ESupportedParamFlags(0)),
    CAnimNode::SParamInfo("DiffuseMultiplier", AnimParamType::LightDiffuseMult, AnimValueType::Float, IAnimNode::ESupportedParamFlags(0)),
    CAnimNode::SParamInfo("HDRDynamic", AnimParamType::LightHDRDynamic, AnimValueType::Float, IAnimNode::ESupportedParamFlags(0)),
    CAnimNode::SParamInfo("SpecularMultiplier", AnimParamType::LightSpecularMult, AnimValueType::Float, IAnimNode::ESupportedParamFlags(0)),
    CAnimNode::SParamInfo("SpecularPercentage", AnimParamType::LightSpecPercentage, AnimValueType::Float, IAnimNode::ESupportedParamFlags(0)),
    CAnimNode::SParamInfo("Visibility", AnimParamType::Visibility, AnimValueType::Bool, IAnimNode::ESupportedParamFlags(0)),
    CAnimNode::SParamInfo("Event", AnimParamType::Event, AnimValueType::Unknown, IAnimNode::ESupportedParamFlags(0))
};

CAnimLightNode::CAnimLightNode(const int id)
    : CAnimEntityNode(id, AnimNodeType::Light)
    , m_fRadius(10.0f)
    , m_fDiffuseMultiplier(1)
    , m_fHDRDynamic(0)
    , m_fSpecularMultiplier(1)
    , m_clrDiffuseColor(255.0f, 255.0f, 255.0f)
    , m_fSpecularPercentage(100.0f)
    , m_bJustActivated(false)
{
}


CAnimLightNode::~CAnimLightNode()
{
}


void CAnimLightNode::Animate(SAnimContext& ec)
{
    CAnimEntityNode::Animate(ec);

    bool bNodeAnimated(false);

    float radius(m_fRadius);
    Vec3 diffuseColor(m_clrDiffuseColor);
    float diffuseMultiplier(m_fDiffuseMultiplier);
    float hdrDynamic(m_fHDRDynamic);
    float specularMultiplier(m_fSpecularMultiplier);
    float specularPercentage(m_fSpecularPercentage);

    GetValueFromTrack(AnimParamType::LightRadius, ec.time, radius);
    GetValueFromTrack(AnimParamType::LightDiffuse,  ec.time, diffuseColor);
    GetValueFromTrack(AnimParamType::LightDiffuseMult, ec.time, diffuseMultiplier);
    GetValueFromTrack(AnimParamType::LightHDRDynamic, ec.time, hdrDynamic);
    GetValueFromTrack(AnimParamType::LightSpecularMult, ec.time, specularMultiplier);
    GetValueFromTrack(AnimParamType::LightSpecPercentage, ec.time, specularPercentage);

    if (m_bJustActivated == true)
    {
        bNodeAnimated = true;
        m_bJustActivated = false;
    }

    if (m_fRadius != radius)
    {
        bNodeAnimated = true;
        m_fRadius = radius;
    }

    if (m_clrDiffuseColor != diffuseColor)
    {
        bNodeAnimated = true;
        m_clrDiffuseColor = diffuseColor;
    }

    if (m_fDiffuseMultiplier != diffuseMultiplier)
    {
        bNodeAnimated = true;
        m_fDiffuseMultiplier = diffuseMultiplier;
    }

    if (m_fHDRDynamic != hdrDynamic)
    {
        bNodeAnimated = true;
        m_fHDRDynamic = hdrDynamic;
    }

    if (m_fSpecularMultiplier != specularMultiplier)
    {
        bNodeAnimated = true;
        m_fSpecularMultiplier = specularMultiplier;
    }

    if (m_fSpecularPercentage != specularPercentage)
    {
        bNodeAnimated = true;
        m_fSpecularPercentage = specularPercentage;
    }

    if (bNodeAnimated && m_pOwner)
    {
        m_bIgnoreSetParam = true;
        if (m_pOwner)
        {
            m_pOwner->OnNodeAnimated(this);
        }
        IEntity* pEntity = GetEntity();
        if (pEntity)
        {
            IScriptTable* scriptObject = pEntity->GetScriptTable();
            if (scriptObject)
            {
                scriptObject->SetValue("Radius", m_fRadius);
                scriptObject->SetValue("clrDiffuse", m_clrDiffuseColor);
                scriptObject->SetValue("fDiffuseMultiplier", m_fDiffuseMultiplier);
                scriptObject->SetValue("fSpecularMultiplier", m_fSpecularMultiplier);
                scriptObject->SetValue("fHDRDynamic", m_fHDRDynamic);
                scriptObject->SetValue("fSpecularPercentage", m_fSpecularPercentage);
            }
        }
        m_bIgnoreSetParam = false;
    }
}


bool CAnimLightNode::GetValueFromTrack(CAnimParamType type, float time, float& value) const
{
    IAnimTrack* pTrack = GetTrackForParameter(type);

    if (pTrack == NULL)
    {
        return false;
    }

    pTrack->GetValue(time, value);

    return true;
}


bool CAnimLightNode::GetValueFromTrack(CAnimParamType type, float time, Vec3& value) const
{
    IAnimTrack* pTrack = GetTrackForParameter(type);

    if (pTrack == NULL)
    {
        return false;
    }

    pTrack->GetValue(time, value);

    return true;
}


void CAnimLightNode::CreateDefaultTracks()
{
    CreateTrack(AnimParamType::Position);
    CreateTrack(AnimParamType::Rotation);
    CreateTrack(AnimParamType::LightRadius);
    CreateTrack(AnimParamType::LightDiffuse);
    CreateTrack(AnimParamType::Event);
}

void CAnimLightNode::InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType)
{
    switch (paramType.GetType())
    {
    case AnimParamType::LightRadius:
        pTrack->SetValue(0, m_fRadius, true);
        break;

    case AnimParamType::LightDiffuse:
        pTrack->SetValue(0, m_clrDiffuseColor, true);
        break;

    case AnimParamType::LightDiffuseMult:
        pTrack->SetValue(0, m_fDiffuseMultiplier, true);
        break;

    case AnimParamType::LightHDRDynamic:
        pTrack->SetValue(0, m_fHDRDynamic, true);
        break;

    case AnimParamType::LightSpecularMult:
        pTrack->SetValue(0, m_fSpecularMultiplier, true);
        break;

    case AnimParamType::LightSpecPercentage:
        pTrack->SetValue(0, m_fSpecularPercentage, true);
        break;
    }
}


void CAnimLightNode::Activate(bool bActivate)
{
    CAnimEntityNode::Activate(bActivate);
    m_bJustActivated = bActivate;
}

/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
void CAnimLightNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    CAnimEntityNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
    if (bLoading)
    {
        xmlNode->getAttr("Radius", m_fRadius);
        xmlNode->getAttr("DiffuseMultiplier", m_fDiffuseMultiplier);
        xmlNode->getAttr("DiffuseColor", m_clrDiffuseColor);
        xmlNode->getAttr("HDRDynamic", m_fHDRDynamic);
        xmlNode->getAttr("SpecularMultiplier", m_fSpecularMultiplier);
        xmlNode->getAttr("SpecularPercentage", m_fSpecularPercentage);
    }
    else
    {
        xmlNode->setAttr("Radius", m_fRadius);
        xmlNode->setAttr("DiffuseMultiplier", m_fDiffuseMultiplier);
        xmlNode->setAttr("DiffuseColor", m_clrDiffuseColor);
        xmlNode->setAttr("HDRDynamic", m_fHDRDynamic);
        xmlNode->setAttr("SpecularMultiplier", m_fSpecularMultiplier);
        xmlNode->setAttr("SpecularPercentage", m_fSpecularPercentage);
    }
}

bool CAnimLightNode::SetParamValue(float time, CAnimParamType param, float value)
{
    switch (param.GetType())
    {
    case AnimParamType::LightRadius:
        m_fRadius = value;
        break;

    case AnimParamType::LightDiffuseMult:
        m_fDiffuseMultiplier = value;
        break;

    case AnimParamType::LightHDRDynamic:
        m_fHDRDynamic = value;
        break;

    case AnimParamType::LightSpecularMult:
        m_fSpecularMultiplier = value;
        break;

    case AnimParamType::LightSpecPercentage:
        m_fSpecularPercentage = value;
        break;
    }

    return CAnimEntityNode::SetParamValue(time, param, value);
}


bool CAnimLightNode::SetParamValue(float time, CAnimParamType param, const Vec3& value)
{
    if (param == AnimParamType::LightDiffuse)
    {
        m_clrDiffuseColor = value;
    }

    return CAnimEntityNode::SetParamValue(time, param, value);
}

unsigned int CAnimLightNode::GetParamCount() const
{
    return NumNodeParams;
}

CAnimParamType CAnimLightNode::GetParamType(unsigned int nIndex) const
{
    if (nIndex >= 0 && nIndex < (int)NumNodeParams)
    {
        return s_nodeParams[nIndex].paramType;
    }

    return AnimParamType::Invalid;
}

bool CAnimLightNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    for (unsigned int i = 0; i < NumNodeParams; i++)
    {
        if (s_nodeParams[i].paramType == paramId)
        {
            info = s_nodeParams[i];
            return true;
        }
    }
    return false;
}