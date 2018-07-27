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
#include <AzCore/Serialization/SerializeContext.h>
#include "ScriptVarNode.h"
#include "AnimTrack.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

#include <ISystem.h>
#include <IScriptSystem.h>

//////////////////////////////////////////////////////////////////////////
CAnimScriptVarNode::CAnimScriptVarNode(const int id)
    : CAnimNode(id, AnimNodeType::ScriptVar)
{
    SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
    m_value = -1e-20f;
}

//////////////////////////////////////////////////////////////////////////
CAnimScriptVarNode::CAnimScriptVarNode()
    : CAnimScriptVarNode(0)
{

}

void CAnimScriptVarNode::OnReset()
{
    m_value = -1e-20f;
}

void CAnimScriptVarNode::OnResume()
{
    OnReset();
}

//////////////////////////////////////////////////////////////////////////
void CAnimScriptVarNode::CreateDefaultTracks()
{
    CreateTrack(AnimParamType::Float);
};

//////////////////////////////////////////////////////////////////////////
unsigned int CAnimScriptVarNode::GetParamCount() const
{
    return 1;
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CAnimScriptVarNode::GetParamType(unsigned int nIndex) const
{
    if (nIndex == 0)
    {
        return AnimParamType::Float;
    }

    return AnimParamType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimScriptVarNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    if (paramId.GetType() == AnimParamType::Float)
    {
        info.flags = IAnimNode::ESupportedParamFlags(0);
        info.name = "Value";
        info.paramType = AnimParamType::Float;
        info.valueType = AnimValueType::Float;
        return true;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
void CAnimScriptVarNode::Animate(SAnimContext& ec)
{
    float value = m_value;

    IAnimTrack* pValueTrack = GetTrackForParameter(AnimParamType::Float);

    if (pValueTrack)
    {
        if (pValueTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled)
        {
            return;
        }

        pValueTrack->GetValue(ec.time, value);
    }

    if (value != m_value)
    {
        m_value = value;
        // Change console var value.
        SetScriptValue();
    }
}

void CAnimScriptVarNode::SetScriptValue()
{
    IScriptSystem* pScriptSystem = gEnv->pMovieSystem->GetSystem()->GetIScriptSystem();
    if (!pScriptSystem)
    {
        return;
    }
    const char* sVarName = GetName();
    const char* sPnt = strchr(sVarName, '.');
    if (sPnt == 0)
    {
        // Global variable.
        pScriptSystem->SetGlobalValue(sVarName, m_value);
    }
    else
    {
        char sTable[256];
        char sName[256];
        azstrcpy(sTable, AZ_ARRAY_SIZE(sTable), sVarName);
        sTable[sPnt - sVarName] = 0;
        azstrcpy(sName, AZ_ARRAY_SIZE(sName), sPnt + 1);

        // In Table value.
        SmartScriptTable pTable;
        if (pScriptSystem->GetGlobalValue(sTable, pTable))
        {
            // Set float value inside table.
            pTable->SetValue(sName, m_value);
        }
    }
}

void CAnimScriptVarNode::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CAnimScriptVarNode, CAnimNode>()
        ->Version(1);
}
