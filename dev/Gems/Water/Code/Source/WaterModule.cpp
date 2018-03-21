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

#include <platform_impl.h>

#include <Water/WaterModule.h>
#include <Water/WaterSystemComponent.h>
#include <Water/WaterOceanComponent.h>
#include <WaterVolumeComponent.h>

#if WATER_GEM_EDITOR
#include "WaterOceanEditor.h"
#include "EditorWaterVolumeComponent.h"
#endif // WATER_GEM_EDITOR

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
#if WATER_GEM_EDITOR
            EditorWaterVolumeComponent::CreateDescriptor(),
            WaterOceanEditor::CreateDescriptor()
#endif // WATER_GEM_EDITOR
        });

#if WATER_GEM_EDITOR
        m_waterConverter = AZStd::make_unique<WaterConverter>();
        m_waterConverter->BusConnect();
#endif // WATER_GEM_EDITOR
    }

    WaterModule::~WaterModule()
    {
#if WATER_GEM_EDITOR
        m_waterConverter->BusDisconnect();
#endif // WATER_GEM_EDITOR
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

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Water_c5083fcf89b24ab68fb0611c01a07b1d, Water::WaterModule)
