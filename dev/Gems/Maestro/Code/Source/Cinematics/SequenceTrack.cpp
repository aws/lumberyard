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
#include "SequenceTrack.h"

//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence Tracks in Component Entity Sequences now occur through AZ::SerializeContext and the Sequence Component
void CSequenceTrack::SerializeKey(ISequenceKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* szSelection;
        szSelection = keyNode->getAttr("node");
        key.szSelection = szSelection;

        AZ::u64 id64;
        if (keyNode->getAttr("sequenceEntityId", id64))
        {
            key.sequenceEntityId = AZ::EntityId(id64);
        }

        if (!keyNode->getAttr("overridetimes", key.bOverrideTimes))
        {
            key.bOverrideTimes = false;
        }

        if (key.bOverrideTimes == true)
        {
            if (!keyNode->getAttr("starttime", key.fStartTime))
            {
                key.fStartTime = 0.0f;
            }
            if (!keyNode->getAttr("endtime", key.fEndTime))
            {
                key.fEndTime  = 0.0f;
            }
        }
        else
        {
            key.fStartTime = 0.0f;
            key.fEndTime = 0.0f;
        }
    }
    else
    {
        keyNode->setAttr("node", key.szSelection.c_str());
        AZ::u64 id64 = static_cast<AZ::u64>(key.sequenceEntityId);
        keyNode->setAttr("sequenceEntityId", id64);

        if (key.bOverrideTimes == true)
        {
            keyNode->setAttr("overridetimes", key.bOverrideTimes);
            keyNode->setAttr("starttime", key.fStartTime);
            keyNode->setAttr("endtime", key.fEndTime);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/// UpConvertKeys()
//
// Legacy levels used sequence names in ISequenceKey. We now use sequenceEntityId.
// This method goes through any legacy keys that used names, finds their corresponding
// ownerIds, fills them in and clears the name.
//
// This can be removed when Legacy Object Sequences are deprecated.
bool CSequenceTrack::UpConvertKeys()
{
    bool keysChanged = false;
    for (int i = m_keys.size(); --i >= 0;)
    {
        ISequenceKey& key = m_keys[i];
        if (!key.szSelection.empty())
        {
            // if for some reason we had a name and a valid ownerId, keep the existing ownerId
            if (!key.sequenceEntityId.IsValid())
            {
                IAnimSequence* sequence = gEnv->pMovieSystem->FindLegacySequenceByName(key.szSelection.c_str());
                if (sequence)
                {
                    key.sequenceEntityId = sequence->GetSequenceEntityId();
                }
            }
            key.szSelection.clear();
            keysChanged = true;
        }
    }
    return keysChanged;
}

//////////////////////////////////////////////////////////////////////////
void CSequenceTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = m_keys[key].fDuration;
    IAnimSequence* sequence = gEnv->pMovieSystem->FindSequence(m_keys[key].sequenceEntityId);
    if (sequence)
    {
        description = sequence->GetName();
    }
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimTrack<ISequenceKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TAnimTrack<ISequenceKey> >()
        ->Version(1)
        ->Field("Flags", &TAnimTrack<ISequenceKey>::m_flags)
        ->Field("Range", &TAnimTrack<ISequenceKey>::m_timeRange)
        ->Field("ParamType", &TAnimTrack<ISequenceKey>::m_nParamType)
        ->Field("Keys", &TAnimTrack<ISequenceKey>::m_keys);
}

//////////////////////////////////////////////////////////////////////////
void CSequenceTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TAnimTrack<ISequenceKey>::Reflect(serializeContext);

    serializeContext->Class<CSequenceTrack, TAnimTrack<ISequenceKey> >()
        ->Version(1);
}