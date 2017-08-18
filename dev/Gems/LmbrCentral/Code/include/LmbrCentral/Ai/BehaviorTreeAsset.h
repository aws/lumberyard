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

#include <AzCore/Asset/AssetCommon.h>

namespace LmbrCentral
{
    /*!
     * Material asset type configuration.
     */
    class BehaviorTreeAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(BehaviorTreeAsset, "{0DB1F34B-EB30-4318-A20B-CF035F419E74}", AZ::Data::AssetData)
        AZ_CLASS_ALLOCATOR(BehaviorTreeAsset, AZ::SystemAllocator, 0);
    };
} // namespace LmbrCentral

