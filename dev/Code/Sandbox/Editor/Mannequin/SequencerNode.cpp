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

#include "StdAfx.h"
#include "SequencerNode.h"

#include "SequencerSequence.h"
#include "SequencerTrack.h"



CSequencerNode::CSequencerNode(CSequencerSequence* sequence, const SControllerDef& controllerDef)
    : m_sequence(sequence)
    , m_controllerDef(controllerDef)
    , m_startExpanded(true)
    , m_muted(false)
    , m_readOnly(false)
{
}


CSequencerNode::~CSequencerNode()
{
}


void CSequencerNode::SetName(const char* name)
{
    m_name = name;
}


const char* CSequencerNode::GetName()
{
    return m_name.c_str();
}


void CSequencerNode::SetSequence(CSequencerSequence* pSequence)
{
    m_sequence = pSequence;
}


CSequencerSequence* CSequencerNode::GetSequence()
{
    return m_sequence;
}


ESequencerNodeType CSequencerNode::GetType() const
{
    return SEQUENCER_NODE_UNDEFINED;
}


IEntity* CSequencerNode::GetEntity()
{
    return NULL;
}


void CSequencerNode::UpdateKeys()
{
    //--- Force an update on all tracks to ensure that they are sorted
    const uint32 numTracks = m_tracks.size();
    for (uint32 i = 0; i < numTracks; i++)
    {
        CSequencerTrack* pTrack = m_tracks[i].track;
        pTrack->UpdateKeys();
    }
}


int CSequencerNode::GetTrackCount() const
{
    return m_tracks.size();
}

void CSequencerNode::UpdateMutedLayerMasks(uint32 mutedAnimLayerMask, uint32 mutedProcLayerMask)
{
    // base class does nothing
}

CSequencerTrack* CSequencerNode::GetTrackByIndex(int nIndex) const
{
    if (m_tracks.size() <= nIndex)
    {
        return NULL;
    }
    return m_tracks[nIndex].track;
}


void CSequencerNode::AddTrack(ESequencerParamType param, CSequencerTrack* track)
{
    TrackDesc td;
    td.paramId = param;
    td.track = track;
    track->SetParameterType(param);
    track->SetTimeRange(GetSequence()->GetTimeRange());
    m_tracks.push_back(td);
}


bool CSequencerNode::RemoveTrack(CSequencerTrack* pTrack)
{
    const uint32 numTracks = m_tracks.size();
    for (uint32 i = 0; i < numTracks; i++)
    {
        if (m_tracks[i].track == pTrack)
        {
            m_tracks.erase(m_tracks.begin() + i);

            return true;
        }
    }
    return false;
}


CSequencerTrack* CSequencerNode::CreateTrack(ESequencerParamType nParamId)
{
    return NULL;
}


CSequencerTrack* CSequencerNode::GetTrackForParameter(ESequencerParamType nParamId) const
{
    const uint32 numTracks = m_tracks.size();
    for (uint32 i = 0; i < numTracks; i++)
    {
        if (m_tracks[i].paramId == nParamId)
        {
            return m_tracks[i].track;
        }
    }
    return NULL;
}


CSequencerTrack* CSequencerNode::GetTrackForParameter(ESequencerParamType nParamId, uint32 index) const
{
    const uint32 numTracks = m_tracks.size();
    uint32 idx = 0;
    for (uint32 i = 0; i < numTracks; i++)
    {
        if (m_tracks[i].paramId == nParamId)
        {
            if (idx == index)
            {
                return m_tracks[i].track;
            }
            idx++;
        }
    }
    return NULL;
}


void CSequencerNode::SetTimeRange(Range timeRange)
{
    const uint32 numTracks = GetTrackCount();
    for (uint32 t = 0; t < numTracks; t++)
    {
        CSequencerTrack* pSeqTrack = GetTrackByIndex(t);
        pSeqTrack->SetTimeRange(timeRange);
    }
}


int CSequencerNode::GetParamCount() const
{
    return 0;
}


bool CSequencerNode::GetParamInfo(int nIndex, SParamInfo& info) const
{
    return false;
}


bool CSequencerNode::GetParamInfoFromId(ESequencerParamType paramId, SParamInfo& info) const
{
    const int paramCount = GetParamCount();
    for (int i = 0; i < paramCount; i++)
    {
        SParamInfo paramInfo;
        GetParamInfo(i, paramInfo);
        if (paramInfo.paramId == paramId)
        {
            info = paramInfo;
            return true;
        }
    }

    return false;
}


bool CSequencerNode::IsParamValid(ESequencerParamType paramId) const
{
    SParamInfo paramInfo;
    const bool isParamValid = GetParamInfoFromId(paramId, paramInfo);
    return isParamValid;
}


bool CSequencerNode::CanAddTrackForParameter(ESequencerParamType paramId) const
{
    if (IsReadOnly())
    {
        return false;
    }
    SParamInfo paramInfo;
    if (!GetParamInfoFromId(paramId, paramInfo))
    {
        return false;
    }

    int flags = 0;
    CSequencerTrack* track = GetTrackForParameter(paramId);
    if (track && !(paramInfo.flags & CSequencerNode::PARAM_MULTIPLE_TRACKS))
    {
        return false;
    }

    return true;
}


void CSequencerNode::InsertMenuOptions(QMenu* menu)
{
}


void CSequencerNode::ClearMenuOptions(QMenu* menu)
{
}


void CSequencerNode::OnMenuOption(int menuOption)
{
}
