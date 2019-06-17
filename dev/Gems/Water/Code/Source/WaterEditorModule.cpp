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
#include <Source/WaterEditorModule.h>

#include <Editor/WaterOceanEditor.h>
#include <Editor/EditorWaterVolumeComponent.h>
#include <Editor/EditorOceanSurfaceDataComponent.h>
#include <Editor/EditorWaterVolumeSurfaceDataComponent.h>

namespace Water
{
    WaterEditorModule::WaterEditorModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            WaterOceanEditor::CreateDescriptor(),
            EditorWaterVolumeComponent::CreateDescriptor(),
            EditorOceanSurfaceDataComponent::CreateDescriptor(),
            EditorWaterVolumeSurfaceDataComponent::CreateDescriptor(),
        });

        m_waterConverter = AZStd::make_unique<WaterConverter>();
    }

    AZ::ComponentTypeList WaterEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = WaterModule::GetRequiredSystemComponents();

        return requiredComponents;
    }

    void WaterEditorModule::OnCrySystemInitialized(ISystem &, const SSystemInitParams & params)
    {
        m_waterConverter->BusConnect();
    }

    void WaterEditorModule::OnCrySystemShutdown(ISystem &)
    {
        m_waterConverter->BusDisconnect();
    }
}

AZ_DECLARE_MODULE_CLASS(WaterEditor, Water::WaterEditorModule)