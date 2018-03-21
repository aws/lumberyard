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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Asset/SimpleAsset.h>

namespace LmbrCentral
{
    /*!
     * Material asset type configuration.
     * Reflect as: AzFramework::SimpleAssetReference<MaterialAsset>
     */
    class MaterialAsset
    {
    public:
        AZ_TYPE_INFO(MaterialAsset, "{F46985B5-F7FF-4FCB-8E8C-DC240D701841}")
        static const char* GetFileFilter() {
            return "*.mtl";
        }
    };

    /*!
    * FBX Material asset type configuration.
    * Reflect as: AzFramework::SimpleAssetReference<FbxMaterialAsset>
    */
    class DccMaterialAsset
    {
    public:
        AZ_TYPE_INFO(DccMaterialAsset, "{C88469CF-21E7-41EB-96FD-BF14FBB05EDC}")
            static const char* GetFileFilter() {
            return "*.dccmtl";
        }
    };

    /*!
     * Texture asset type configuration.
     * Reflect as: AzFramework::SimpleAssetReference<TextureAsset>
     */
    class TextureAsset
    {
    public:
        AZ_TYPE_INFO(TextureAsset, "{59D5E20B-34DB-4D8E-B867-D33CC2556355}")
        static const char* GetFileFilter()
        {
            return "*.dds; *.tif; *.bmp; *.gif; *.jpg; *.jpeg; *.jpe; *.tga; *.png; *.swf; *.gfx; *.sfd; *.sprite";
        }
    };
} // namespace LmbrCentral

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>, "{B7B8ECC7-FF89-4A76-A50E-4C6CA2B6E6B4}")
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::SimpleAssetReference<LmbrCentral::DccMaterialAsset>, "{E865C742-A063-47A3-BCE1-E724A8D4B66D}")
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>, "{68E92460-5C0C-4031-9620-6F1A08763243}")
}
