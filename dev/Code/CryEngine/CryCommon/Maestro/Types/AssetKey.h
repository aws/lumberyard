/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <AnimKey.h>

namespace AZ
{
    struct IAssetKey
        : public IKey
    {
        IAssetKey() = default;

        Data::AssetId m_assetId;
        AZStd::string m_assetTypeName;
    };

    AZ_TYPE_INFO_SPECIALIZE(IAssetKey, "{45B9B613-6BD4-4BC2-AC7D-3630865C8693}");
}