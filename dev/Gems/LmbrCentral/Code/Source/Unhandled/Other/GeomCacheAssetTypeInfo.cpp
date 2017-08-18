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

#include "StdAfx.h"
#include "GeomCacheAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType geomCacheType("{EBC96071-E960-41B6-B3E3-328F515AE5DA}");

    GeomCacheAssetTypeInfo::~GeomCacheAssetTypeInfo()
    {
        Unregister();
    }

    void GeomCacheAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(geomCacheType);
    }

    void GeomCacheAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(geomCacheType);
    }

    AZ::Data::AssetType GeomCacheAssetTypeInfo::GetAssetType() const
    {
        return geomCacheType;
    }

    const char* GeomCacheAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Geom Cache";
    }

    const char* GeomCacheAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
} // namespace LmbrCentral
