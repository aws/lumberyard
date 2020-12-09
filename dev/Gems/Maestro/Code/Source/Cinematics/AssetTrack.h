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
#pragma once

#include <Maestro/Types/AssetKey.h>
#include "IMovieSystem.h"
#include "AnimTrack.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/** Asset Id track
*/
class CAssetTrack
    : public TAnimTrack<AZ::IAssetKey>
{
public:
    AZ_CLASS_ALLOCATOR(CAssetTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CAssetTrack, "{DD2B10C2-FC8B-41B7-9BED-E3B27BBC3738}", IAnimTrack);

    CAssetTrack();

    AnimValueType GetValueType() override;

    bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true) override;
    void SerializeKey(AZ::IAssetKey& key, XmlNodeRef& keyNode, bool bLoading) override;
    static void Reflect(AZ::SerializeContext* serializeContext);

    void GetKeyInfo(int key, const char*& description, float& duration);

    void GetValue(float time, AZ::Data::AssetId& value) override;
    void SetValue(float time, const AZ::Data::AssetId& value, bool bDefault = false) override;

    void SetAssetTypeName(const AZStd::string& assetTypeName) override;
    void SetKey(int index, IKey* key) override;
private:
    AZ::Data::AssetId m_defaultAsset;
    AZStd::string m_assetTypeName;
};
