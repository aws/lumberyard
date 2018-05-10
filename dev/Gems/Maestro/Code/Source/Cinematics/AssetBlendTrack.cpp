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
#include <AzFramework/StringFunc/StringFunc.h>

#include <AnimKey.h>
#include <Maestro/Types/AssetBlendKey.h>

#include "AssetBlendTrack.h"
#include "Maestro/Types/AnimValueType.h"

#define LOOP_TRANSITION_TIME 1.0f


//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
bool CAssetBlendTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    return TAnimTrack<AZ::IAssetBlendKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
}

void CAssetBlendTrack::SerializeKey(AZ::IAssetBlendKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        // Read the AssetId made up of the guid and the sub id.
        key.m_assetId.SetInvalid();
        const char* assetIdGuidStr = keyNode->getAttr("assetIdGuid");
        if (assetIdGuidStr != nullptr && assetIdGuidStr[0] != 0)
        {
            AZ::Uuid guid(assetIdGuidStr, strlen(assetIdGuidStr));
            AZ::u32 assetIdSubId = 0;
            keyNode->getAttr("assetIdSubId", assetIdSubId);
            key.m_assetId = AZ::Data::AssetId(guid, assetIdSubId);
        }

        const char* descriptionIdStr = keyNode->getAttr("description");
        if (descriptionIdStr != nullptr)
        {
            key.m_description = descriptionIdStr;
        }

        key.m_duration = 0;
        key.m_endTime = 0;
        key.m_startTime = 0;
        key.m_bLoop = false;
        key.m_speed = 1;
        keyNode->getAttr("length", key.m_duration);
        keyNode->getAttr("end", key.m_endTime);
        keyNode->getAttr("speed", key.m_speed);
        keyNode->getAttr("loop", key.m_bLoop);
        keyNode->getAttr("start", key.m_startTime);
    }
    else
    {
        if (key.m_assetId.IsValid())
        {            
            XmlString temp = key.m_assetId.m_guid.ToString<AZStd::string>().c_str();
            keyNode->setAttr("assetIdGuid", temp);
            keyNode->setAttr("assetIdSubId", key.m_assetId.m_subId);
        }
        if (!key.m_description.empty())
        {
            XmlString temp = key.m_description.c_str();
            keyNode->setAttr("description", temp);
        }
        if (key.m_duration > 0)
        {
            keyNode->setAttr("length", key.m_duration);
        }
        if (key.m_endTime > 0)
        {
            keyNode->setAttr("end", key.m_endTime);
        }
        if (key.m_speed != 1)
        {
            keyNode->setAttr("speed", key.m_speed);
        }
        if (key.m_bLoop)
        {
            keyNode->setAttr("loop", key.m_bLoop);
        }
        if (key.m_startTime != 0)
        {
            keyNode->setAttr("start", key.m_startTime);
        }
    }
}

void CAssetBlendTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = 0;
    if (m_keys[key].m_assetId.IsValid())
    {
        description = m_keys[key].m_description.c_str();

        if (m_keys[key].m_bLoop)
        {
            float lastTime = m_timeRange.end;
            if (key + 1 < (int)m_keys.size())
            {
                lastTime = m_keys[key + 1].time;
            }
            // duration is unlimited but cannot last past end of track or time of next key on track.
            duration = lastTime - m_keys[key].time;
        }
        else
        {
            if (m_keys[key].m_speed == 0)
            {
                m_keys[key].m_speed = 1.0f;
            }
            duration = m_keys[key].GetActualDuration();
        }
    }
}

float CAssetBlendTrack::GetKeyDuration(int key) const
{
    assert(key >= 0 && key < (int)m_keys.size());
    const float EPSILON = 0.001f;
    if (m_keys[key].m_bLoop)
    {
        float lastTime = m_timeRange.end;
        if (key + 1 < (int)m_keys.size())
        {
            // EPSILON is required to ensure the correct ordering when getting nearest keys.
            lastTime = m_keys[key + 1].time + std::min(LOOP_TRANSITION_TIME,
                    GetKeyDuration(key + 1) - EPSILON);
        }
        // duration is unlimited but cannot last past end of track or time of next key on track.
        return std::max(lastTime - m_keys[key].time, 0.0f);
    }
    else
    {
        return m_keys[key].GetActualDuration();
    }
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimTrack<AZ::IAssetBlendKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TAnimTrack<AZ::IAssetBlendKey> >()
        ->Version(1)
        ->Field("Flags", &TAnimTrack<AZ::IAssetBlendKey>::m_flags)
        ->Field("Range", &TAnimTrack<AZ::IAssetBlendKey>::m_timeRange)
        ->Field("ParamType", &TAnimTrack<AZ::IAssetBlendKey>::m_nParamType)
        ->Field("Keys", &TAnimTrack<AZ::IAssetBlendKey>::m_keys);
}

//////////////////////////////////////////////////////////////////////////
AnimValueType CAssetBlendTrack::GetValueType()
{
    return AnimValueType::AssetBlend; 
}

//////////////////////////////////////////////////////////////////////////
void CAssetBlendTrack::GetValue(float time, AZ::Data::AssetBlends<AZ::Data::AssetData>& value)
{
    // Start by clearing all the assets.
    m_assetBlend.m_assetBlends.clear();

    // Check each key to see if it has an asset that is in time range right now.
    for (auto key : m_keys)
    {
        if (key.IsInRange(time) && key.m_assetId.IsValid())
        {           
            m_assetBlend.m_assetBlends.push_back(AZ::Data::AssetBlend(key.m_assetId, key.time));
        }
    }

    // Return the updated assetBlend
    value = m_assetBlend;
}

//////////////////////////////////////////////////////////////////////////
float CAssetBlendTrack::GetEndTime() const
{
    return m_timeRange.end; 
}

//////////////////////////////////////////////////////////////////////////
void CAssetBlendTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TAnimTrack<AZ::IAssetBlendKey>::Reflect(serializeContext);

    serializeContext->Class<CAssetBlendTrack, TAnimTrack<AZ::IAssetBlendKey> >()
        ->Version(1);
}
