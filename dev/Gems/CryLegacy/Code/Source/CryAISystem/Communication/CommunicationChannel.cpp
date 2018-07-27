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

#include "CryLegacy_precompiled.h"
#include "CommunicationChannel.h"


CommunicationChannel::CommunicationChannel(const CommunicationChannel::Ptr& parent,
    const SCommunicationChannelParams& params,
    const CommChannelID& channelID)
    : m_parent(parent)
    , m_minSilence(params.minSilence)
    , m_silence(0.0f)
    , m_flushSilence(params.flushSilence)
    , m_actorMinSilence(params.actorMinSilence)
    , m_priority(params.priority)
    , m_id(channelID)
    , m_occupied(false)
    , m_type(params.type)
    , m_ignoreActorSilence(params.ignoreActorSilence)
{}

void CommunicationChannel::Update(float updateTime)
{
    if (!m_occupied && (m_silence > 0.0f))
    {
        m_silence -= updateTime;
    }
}

void CommunicationChannel::Occupy(bool occupy, float minSilence)
{
    assert((occupy && IsFree()) || (!occupy && !IsFree()));

    if (m_occupied && !occupy)
    {
        m_silence = (minSilence >= 0.0f) ? minSilence : m_minSilence;
    }
    m_occupied = occupy;

    if (m_parent)
    {
        m_parent->Occupy(occupy, minSilence);
    }
}

bool CommunicationChannel::IsFree() const
{
    if (m_parent && !m_parent->IsFree())
    {
        return false;
    }

    return !m_occupied && (m_silence <= 0.0f);
}

void CommunicationChannel::Clear()
{
    m_occupied = false;
    m_silence = 0.0;
}

void CommunicationChannel::ResetSilence()
{
    if (m_parent && !m_parent->IsOccupied())
    {
        m_parent->ResetSilence();
    }

    m_silence = m_flushSilence;
}

