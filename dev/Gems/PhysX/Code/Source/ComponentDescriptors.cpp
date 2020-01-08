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

#include "PhysX_precompiled.h"

#include <ComponentDescriptors.h>
#include <Source/SystemComponent.h>
#include <Source/TerrainComponent.h>
#include <Source/RigidBodyComponent.h>
#include <Source/BaseColliderComponent.h>
#include <Source/MeshColliderComponent.h>
#include <Source/BoxColliderComponent.h>
#include <Source/SphereColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>
#include <Source/ShapeColliderComponent.h>
#include <Source/ForceRegionComponent.h>

namespace PhysX
{
    AZStd::list<AZ::ComponentDescriptor*> GetDescriptors()
    {
        AZStd::list<AZ::ComponentDescriptor*> descriptors =
        {
            SystemComponent::CreateDescriptor(),
            TerrainComponent::CreateDescriptor(),
            RigidBodyComponent::CreateDescriptor(),
            BaseColliderComponent::CreateDescriptor(),
            MeshColliderComponent::CreateDescriptor(),
            BoxColliderComponent::CreateDescriptor(),
            SphereColliderComponent::CreateDescriptor(),
            CapsuleColliderComponent::CreateDescriptor(),
            ShapeColliderComponent::CreateDescriptor(),
            ForceRegionComponent::CreateDescriptor()
        };

        return descriptors;
    }
} // namespace PhysX
