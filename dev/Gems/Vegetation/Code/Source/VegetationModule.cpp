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

#include "Vegetation_precompiled.h"

#if !defined(AZ_MONOLITHIC_BUILD)
#include <platform_impl.h> // must be included once per DLL so things from CryCommon will function
#endif

#include <VegetationModule.h>

#include <Components/AreaBlenderComponent.h>
#include <Components/BlockerComponent.h>
#include <Components/DescriptorListCombinerComponent.h>
#include <Components/DescriptorListComponent.h>
#include <Components/DescriptorWeightSelectorComponent.h>
#include <Components/DistanceBetweenFilterComponent.h>
#include <Components/DistributionFilterComponent.h>
#include <Components/LevelSettingsComponent.h>
#include <Components/MeshBlockerComponent.h>
#include <Components/PositionModifierComponent.h>
#include <Components/ReferenceShapeComponent.h>
#include <Components/RotationModifierComponent.h>
#include <Components/ScaleModifierComponent.h>
#include <Components/ShapeIntersectionFilterComponent.h>
#include <Components/SlopeAlignmentModifierComponent.h>
#include <Components/SpawnerComponent.h>
#include <Components/SurfaceAltitudeFilterComponent.h>
#include <Components/SurfaceMaskDepthFilterComponent.h>
#include <Components/SurfaceMaskFilterComponent.h>
#include <Components/SurfaceSlopeFilterComponent.h>
#include <Debugger/DebugComponent.h>
#include <Debugger/AreaDebugComponent.h>
#include <AreaSystemComponent.h>
#include <InstanceSystemComponent.h>
#include <VegetationSystemComponent.h>
#include <Components/StaticVegetationBlockerComponent.h>
#include <StaticVegetationSystemComponent.h>
#include <DebugSystemComponent.h>

namespace Vegetation
{
    VegetationModule::VegetationModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {

            AreaBlenderComponent::CreateDescriptor(),
            BlockerComponent::CreateDescriptor(),
            DescriptorListCombinerComponent::CreateDescriptor(),
            DescriptorListComponent::CreateDescriptor(),
            DescriptorWeightSelectorComponent::CreateDescriptor(),
            DistanceBetweenFilterComponent::CreateDescriptor(),
            DistributionFilterComponent::CreateDescriptor(),
            LevelSettingsComponent::CreateDescriptor(),
            MeshBlockerComponent::CreateDescriptor(),
            PositionModifierComponent::CreateDescriptor(),
            ReferenceShapeComponent::CreateDescriptor(),
            RotationModifierComponent::CreateDescriptor(),
            ScaleModifierComponent::CreateDescriptor(),
            ShapeIntersectionFilterComponent::CreateDescriptor(),
            SlopeAlignmentModifierComponent::CreateDescriptor(),
            SpawnerComponent::CreateDescriptor(),
            SurfaceAltitudeFilterComponent::CreateDescriptor(),
            SurfaceMaskDepthFilterComponent::CreateDescriptor(),
            SurfaceMaskFilterComponent::CreateDescriptor(),
            SurfaceSlopeFilterComponent::CreateDescriptor(),
            AreaSystemComponent::CreateDescriptor(),
            InstanceSystemComponent::CreateDescriptor(),
            VegetationSystemComponent::CreateDescriptor(),
            StaticVegetationBlockerComponent::CreateDescriptor(),
            StaticVegetationSystemComponent::CreateDescriptor(),
            DebugComponent::CreateDescriptor(),
            DebugSystemComponent::CreateDescriptor(),
            AreaDebugComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList VegetationModule::GetRequiredSystemComponents() const
    {
        // [LY-90913] Revisit the need for these to be required components if/when other components ever get created that fulfill the same
        // service and interface as these.  Until then, making them required improves usability because users will be guided to add all the 
        // dependent system components that vegetation needs.
        return AZ::ComponentTypeList{
            azrtti_typeid<VegetationSystemComponent>(),
            azrtti_typeid<AreaSystemComponent>(),
            azrtti_typeid<InstanceSystemComponent>(),
            azrtti_typeid<StaticVegetationSystemComponent>(),
            azrtti_typeid<DebugSystemComponent>(),
        };
    }
}

#if !defined(VEGETATION_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Vegetation_f394e7cf54424bba89615381bba9511b, Vegetation::VegetationModule)
#endif
