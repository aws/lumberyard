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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>

namespace LmbrCentral
{
    class ParticleAsset
        : public AZ::Data::AssetData
    {
    public:

        AZ_RTTI(ParticleAsset, "{6EB56B55-1B58-4EE3-A268-27680338AE56}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ParticleAsset, AZ::SystemAllocator, 0);

        AZStd::vector<AZStd::string> m_emitterNames;
    };
} // namespace LmbrCentral
