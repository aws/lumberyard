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

#include "LmbrCentral_precompiled.h"
#include "SimpleAnimationAssetTypeInfo.h"

#include <LmbrCentral/Rendering/MaterialAsset.h>

namespace LmbrCentral
{
    static AZ::Data::AssetType simpleAnimationType("{6023CFF8-FCBA-4528-A8BF-6E0E10B9AB9C}");

    SimpleAnimationAssetTypeInfo::~SimpleAnimationAssetTypeInfo()
    {
        Unregister();
    }

    void SimpleAnimationAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(simpleAnimationType);
    }

    void SimpleAnimationAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(simpleAnimationType);
    }

    AZ::Data::AssetType SimpleAnimationAssetTypeInfo::GetAssetType() const
    {
        return simpleAnimationType;
    }

    const char* SimpleAnimationAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Simple Animation";
    }

    const char* SimpleAnimationAssetTypeInfo::GetGroup() const
    {
        return "Animation";
    }
} // namespace LmbrCentral
