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

// Description : CryMovie animation node for geometry caches


#include "Maestro_precompiled.h"
#if defined(USE_GEOM_CACHES)
#include "AnimGeomCacheNode.h"
#include "TimeRangesTrack.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

namespace AnimGeomCacheNode
{
    bool s_geomCacheNodeParamsInitialized = false;
    std::vector<CAnimNode::SParamInfo> s_geomCacheNodeParams;

    void AddSupportedParams(const char* sName, AnimParamType paramType, AnimValueType valueType)
    {
        CAnimNode::SParamInfo param;
        param.name = sName;
        param.paramType = paramType;
        param.valueType = valueType;
        param.flags = IAnimNode::ESupportedParamFlags(0);
        s_geomCacheNodeParams.push_back(param);
    }
};

CAnimGeomCacheNode::CAnimGeomCacheNode(const int id)
    : CAnimEntityNode(id, AnimNodeType::GeomCache)
    , m_bActive(false)
{
    CAnimGeomCacheNode::Initialize();
}

CAnimGeomCacheNode::~CAnimGeomCacheNode()
{
}

void CAnimGeomCacheNode::Initialize()
{
    if (!AnimGeomCacheNode::s_geomCacheNodeParamsInitialized)
    {
        AnimGeomCacheNode::s_geomCacheNodeParamsInitialized = true;
        AnimGeomCacheNode::s_geomCacheNodeParams.reserve(1);
        AnimGeomCacheNode::AddSupportedParams("Animation", AnimParamType::TimeRanges, AnimValueType::Unknown);
    }
}

void CAnimGeomCacheNode::Animate(SAnimContext& animContext)
{
    IGeomCacheRenderNode* pGeomCacheRenderNode = GetGeomCacheRenderNode();
    if (pGeomCacheRenderNode)
    {
        const unsigned int trackCount = NumTracks();
        for (unsigned int paramIndex = 0; paramIndex < trackCount; ++paramIndex)
        {
            CAnimParamType paramType = m_tracks[paramIndex]->GetParameterType();
            IAnimTrack* pTrack = m_tracks[paramIndex].get();

            if (pTrack && paramType == AnimParamType::TimeRanges)
            {
                CTimeRangesTrack* pTimeRangesTrack = static_cast<CTimeRangesTrack*>(pTrack);

                if ((pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled) || pTrack->IsMasked(animContext.trackMask) || pTrack->GetNumKeys() == 0)
                {
                    continue;
                }

                const int currentKeyIndex = pTimeRangesTrack->GetActiveKeyIndexForTime(animContext.time);
                if (currentKeyIndex != -1)
                {
                    ITimeRangeKey key;
                    pTimeRangesTrack->GetKey(currentKeyIndex, &key);
                    pGeomCacheRenderNode->SetLooping(key.m_bLoop);

                    const float playbackTime = (animContext.time - key.time)  * key.m_speed + key.m_startTime;
                    pGeomCacheRenderNode->SetPlaybackTime(std::min(playbackTime, key.m_endTime));

                    if (playbackTime > key.m_endTime)
                    {
                        pGeomCacheRenderNode->StopStreaming();
                    }
                }
                else
                {
                    pGeomCacheRenderNode->SetLooping(false);
                    pGeomCacheRenderNode->SetPlaybackTime(0.0f);
                    pGeomCacheRenderNode->StopStreaming();
                }
            }
        }
    }

    CAnimEntityNode::Animate(animContext);
}

void CAnimGeomCacheNode::CreateDefaultTracks()
{
    CreateTrack(AnimParamType::TimeRanges);
}

void CAnimGeomCacheNode::OnReset()
{
    CAnimNode::OnReset();
    m_bActive = false;
}

void CAnimGeomCacheNode::Activate(bool bActivate)
{
    m_bActive = bActivate;

    IGeomCacheRenderNode* pGeomCacheRenderNode = GetGeomCacheRenderNode();
    if (pGeomCacheRenderNode)
    {
        if (!bActivate)
        {
            if (gEnv->IsEditor() && gEnv->IsEditorGameMode() == false)
            {
                pGeomCacheRenderNode->SetLooping(false);
                pGeomCacheRenderNode->SetPlaybackTime(0.0f);
            }

            pGeomCacheRenderNode->StopStreaming();
        }
    }
}

unsigned int CAnimGeomCacheNode::GetParamCount() const
{
    return AnimGeomCacheNode::s_geomCacheNodeParams.size() + CAnimEntityNode::GetParamCount();
}

CAnimParamType CAnimGeomCacheNode::GetParamType(unsigned int nIndex) const
{
    const unsigned int numOwnParams = (unsigned int)AnimGeomCacheNode::s_geomCacheNodeParams.size();

    if (nIndex >= 0 && nIndex < numOwnParams)
    {
        return AnimGeomCacheNode::s_geomCacheNodeParams[nIndex].paramType;
    }
    else if (nIndex >= numOwnParams && nIndex < CAnimEntityNode::GetParamCount() + numOwnParams)
    {
        return CAnimEntityNode::GetParamType(nIndex - numOwnParams);
    }

    return AnimParamType::Invalid;
}

bool CAnimGeomCacheNode::GetParamInfoFromType(const CAnimParamType& paramType, SParamInfo& info) const
{
    for (size_t i = 0; i < AnimGeomCacheNode::s_geomCacheNodeParams.size(); ++i)
    {
        if (AnimGeomCacheNode::s_geomCacheNodeParams[i].paramType == paramType)
        {
            info = AnimGeomCacheNode::s_geomCacheNodeParams[i];
            return true;
        }
    }

    return CAnimEntityNode::GetParamInfoFromType(paramType, info);
}

IGeomCacheRenderNode* CAnimGeomCacheNode::GetGeomCacheRenderNode()
{
    IEntity* pEntity = GetEntity();
    if (pEntity)
    {
        return pEntity->GetGeomCacheRenderNode(0);
    }

    return NULL;
}

void CAnimGeomCacheNode::PrecacheDynamic(float startTime)
{
    const ICVar* pBufferAheadTimeCVar = gEnv->pConsole->GetCVar("e_GeomCacheBufferAheadTime");
    const float startPrecacheTime = pBufferAheadTimeCVar ? pBufferAheadTimeCVar->GetFVal() : 1.0f;

    IGeomCacheRenderNode* pGeomCacheRenderNode = GetGeomCacheRenderNode();
    if (pGeomCacheRenderNode)
    {
        const unsigned int trackCount = NumTracks();
        for (unsigned int paramIndex = 0; paramIndex < trackCount; ++paramIndex)
        {
            CAnimParamType paramType = m_tracks[paramIndex]->GetParameterType();
            IAnimTrack* pTrack = m_tracks[paramIndex].get();

            if (pTrack && paramType == AnimParamType::TimeRanges)
            {
                CTimeRangesTrack* pTimeRangesTrack = static_cast<CTimeRangesTrack*>(pTrack);

                if ((pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled) || pTrack->GetNumKeys() == 0)
                {
                    continue;
                }

                const int currentKeyIndex = pTimeRangesTrack->GetActiveKeyIndexForTime(startTime + startPrecacheTime);
                if (currentKeyIndex != -1)
                {
                    ITimeRangeKey key;
                    pTimeRangesTrack->GetKey(currentKeyIndex, &key);

                    if (startTime < key.time + key.m_startTime)
                    {
                        pGeomCacheRenderNode->SetLooping(key.m_bLoop);
                        pGeomCacheRenderNode->StartStreaming(key.m_startTime);
                    }
                }
            }
        }
    }

    CAnimEntityNode::PrecacheDynamic(startTime);
}
#endif