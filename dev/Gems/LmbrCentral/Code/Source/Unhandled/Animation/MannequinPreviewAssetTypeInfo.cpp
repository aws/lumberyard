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
#include "MannequinPreviewAssetTypeInfo.h"

#include <LmbrCentral/Rendering/MaterialAsset.h>

namespace LmbrCentral
{
    static AZ::Data::AssetType mannequinPreviewType("{1FFF61EE-044A-4A72-87D0-60870FF02C58}");

    MannequinPreviewAssetTypeInfo::~MannequinPreviewAssetTypeInfo()
    {
        Unregister();
    }

    void MannequinPreviewAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(mannequinPreviewType);
    }

    void MannequinPreviewAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(mannequinPreviewType);
    }

    AZ::Data::AssetType MannequinPreviewAssetTypeInfo::GetAssetType() const
    {
        return mannequinPreviewType;
    }

    const char* MannequinPreviewAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Mannequin Preview";
    }

    const char* MannequinPreviewAssetTypeInfo::GetGroup() const
    {
        return "Animation";
    }
    const char* MannequinPreviewAssetTypeInfo::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/Mannequin.png";
    }
} // namespace LmbrCentral
