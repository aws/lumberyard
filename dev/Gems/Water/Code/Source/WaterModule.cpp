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

#include "Water_precompiled.h"

#if !defined(AZ_MONOLITHIC_BUILD)
#include <platform_impl.h> // must be included once per DLL so things from CryCommon will function
#endif

#include <Water/WaterModule.h>
#include <Water/WaterSystemComponent.h>
#include <Water/WaterOceanComponent.h>
#include <WaterVolumeComponent.h>
#include <Source/OceanSurfaceDataComponent.h>
#include <Source/WaterVolumeSurfaceDataComponent.h>

#include <IGem.h>

namespace Water
{
    WaterModule::WaterModule()
        : CryHooksModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            WaterSystemComponent::CreateDescriptor(),
            WaterOceanComponent::CreateDescriptor(),
            WaterVolumeComponent::CreateDescriptor(),
            OceanSurfaceDataComponent::CreateDescriptor(),
            WaterVolumeSurfaceDataComponent::CreateDescriptor(),
        });
    }

    /**
     * Add required SystemComponents to the SystemEntity.
     */
    AZ::ComponentTypeList WaterModule::GetRequiredSystemComponents() const 
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<WaterSystemComponent>(),
        };
    }
}

#if !defined(WATER_GEM_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Water_c5083fcf89b24ab68fb0611c01a07b1d, Water::WaterModule)
#endif
