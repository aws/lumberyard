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

#include "Maestro_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>

#include "AssetTrack.h"
#include "Maestro/Types/AnimValueType.h"

//////////////////////////////////////////////////////////////////////////
CAssetTrack::CAssetTrack()
    : m_defaultAsset()
{
}

//////////////////////////////////////////////////////////////////////////
void CAssetTrack::GetKeyInfo(int index, const char*& description, float& duration)
{
    description = 0;
    duration = 0;
}

//////////////////////////////////////////////////////////////////////////
AnimValueType CAssetTrack::GetValueType()
{
    return AnimValueType::AssetId; 
}

/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
bool CAssetTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    return TAnimTrack<AZ::IAssetKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
}

void CAssetTrack::GetValue(float time, AZ::Data::AssetId& value)
{
    value = m_defaultAsset;

    // This will make sure keys are sorted and in a valid state
    CheckValid();

    AZ::IAssetKey activeKey;
    if (GetActiveKey(time, &activeKey) >= 0)
    {
        // If the key's asset id is valid, it's an override. Otherwise, it means we're returning to the default.
        if (activeKey.m_assetId.IsValid())
        {
            value = activeKey.m_assetId;
        }
    }
}

void CAssetTrack::SetValue(float time, const AZ::Data::AssetId& value, bool bDefault /*= false*/)
{
    if (bDefault)
    {
        m_defaultAsset = value;
    }
    Invalidate();
}

void CAssetTrack::SetAssetTypeName(const AZStd::string& assetTypeName)
{
    m_assetTypeName = assetTypeName;
}

void CAssetTrack::SetKey(int index, IKey* key)
{
    if (key)
    {
        AZ::IAssetKey* assetKey = reinterpret_cast<AZ::IAssetKey*>(key);
        assetKey->m_assetTypeName = m_assetTypeName;
    }

    TAnimTrack<AZ::IAssetKey>::SetKey(index, key);
}

void CAssetTrack::SerializeKey(AZ::IAssetKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        key.m_assetId.SetInvalid();
        const char* assetIdGuidStr = keyNode->getAttr("assetIdGuid");
        if (assetIdGuidStr && assetIdGuidStr[0])
        {
            AZ::Uuid guid(assetIdGuidStr, strlen(assetIdGuidStr));
            AZ::u32 assetIdSubId = 0;
            keyNode->getAttr("assetIdSubId", assetIdSubId);
            key.m_assetId = AZ::Data::AssetId(guid, assetIdSubId);
        }
        const char* assetTypeName = keyNode->getAttr("assetTypeName");
        key.m_assetTypeName = assetTypeName;
    }
    else
    {
        if (key.m_assetId.IsValid())
        {
            XmlString temp = key.m_assetId.m_guid.ToString<AZStd::string>().c_str();
            keyNode->setAttr("assetIdGuid", temp);
            keyNode->setAttr("assetIdSubId", key.m_assetId.m_subId);
            XmlString assetTypeName = key.m_assetTypeName.c_str();
            keyNode->setAttr("assetTypeName", assetTypeName);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimTrack<AZ::IAssetKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<AZ::IAssetKey, IKey>()
        ->Version(2)
        ->Field("AssetId", &AZ::IAssetKey::m_assetId)
        ->Field("AssetTypeName", &AZ::IAssetKey::m_assetTypeName);

    serializeContext->Class<TAnimTrack<AZ::IAssetKey> >()
        ->Version(1)
        ->Field("Flags", &TAnimTrack<AZ::IAssetKey>::m_flags)
        ->Field("Range", &TAnimTrack<AZ::IAssetKey>::m_timeRange)
        ->Field("ParamType", &TAnimTrack<AZ::IAssetKey>::m_nParamType)
        ->Field("Keys", &TAnimTrack<AZ::IAssetKey>::m_keys)
        ->Field("Id", &TAnimTrack<AZ::IAssetKey>::m_id);
}

//////////////////////////////////////////////////////////////////////////
void CAssetTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TAnimTrack<AZ::IAssetKey>::Reflect(serializeContext);

    serializeContext->Class<CAssetTrack, TAnimTrack<AZ::IAssetKey> >()
        ->Version(1)
        ->Field("DefaultValue", &CAssetTrack::m_defaultAsset);
}
