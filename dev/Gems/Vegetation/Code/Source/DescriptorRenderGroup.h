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

#include <IEntityRenderState.h>
#include <StatObjBus.h>
#include <Vegetation/Descriptor.h>
#include <AzCore/Asset/AssetCommon.h>

struct I3DEngine;

namespace Vegetation
{
    /**
    * Automatic registration of static object instance group and settings 
    */
    class DescriptorRenderGroup 
    {
    public:
        AZ_CLASS_ALLOCATOR(DescriptorRenderGroup, AZ::SystemAllocator, 0);

        DescriptorRenderGroup(const Descriptor& descriptor, I3DEngine* engine);
        ~DescriptorRenderGroup();

        // 3D engine management
        bool IsReady() const;

        StatInstGroupId GetId() const;

        static bool ShouldDescriptorsShareGroup(const Descriptor& lhs, const Descriptor& rhs);

    private:
        DescriptorRenderGroup(const DescriptorRenderGroup& o) = delete;
        DescriptorRenderGroup& operator=(const DescriptorRenderGroup& o) = delete;

        // Current 3D engine group Id of this object
        StatInstGroupId m_id = StatInstGroupEvents::s_InvalidStatInstGroupId;
        I3DEngine* m_engine = nullptr;
    };

    using DescriptorRenderGroupPtr = AZStd::shared_ptr<DescriptorRenderGroup>;
}
