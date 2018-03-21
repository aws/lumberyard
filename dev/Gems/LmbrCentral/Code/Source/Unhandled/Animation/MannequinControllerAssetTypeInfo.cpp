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
#include "MannequinControllerAssetTypeInfo.h"

#include <LmbrCentral/Animation/MannequinAsset.h>

namespace LmbrCentral
{
    MannequinControllerAssetTypeInfo::~MannequinControllerAssetTypeInfo()
    {
        Unregister();
    }

    void MannequinControllerAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<MannequinControllerDefinitionAsset>::Uuid());
    }

    void MannequinControllerAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<MannequinControllerDefinitionAsset>::Uuid());
    }

    AZ::Data::AssetType MannequinControllerAssetTypeInfo::GetAssetType() const
    {
        return AZ::AzTypeInfo<MannequinControllerDefinitionAsset>::Uuid();
    }

    const char* MannequinControllerAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Mannequin Controller";
    }

    const char* MannequinControllerAssetTypeInfo::GetGroup() const
    {
        return "Animation";
    }
    const char* MannequinControllerAssetTypeInfo::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/Mannequin.png";
    }
} // namespace LmbrCentral
