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
#include "EditorDebugComponent.h"

#include <AzCore/Serialization/Utils.h>
#include <AzCore/base.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <VegetationSystemComponent.h>

namespace Vegetation
{
    void EditorDebugComponent::MergedMeshDebug::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<MergedMeshDebug>()
                ->Version(0)
                ->Field("stressInfo", &MergedMeshDebug::m_stressInfo)
                ->Field("printState", &MergedMeshDebug::m_printState)
                ->Field("instanceLines", &MergedMeshDebug::m_instanceLines)
                ->Field("instanceAABBs", &MergedMeshDebug::m_instanceAABBs)
                ->Field("spinesFilter", &MergedMeshDebug::m_spinesFilter)
                ->Field("calculatedWind", &MergedMeshDebug::m_calculatedWind)
                ->Field("influencingColliders", &MergedMeshDebug::m_influencingColliders)
                ->Field("spinesAsLines", &MergedMeshDebug::m_spinesAsLines)
                ->Field("simulatedSpines", &MergedMeshDebug::m_simulatedSpines)
                ->Field("spinesLOD", &MergedMeshDebug::m_spinesLOD)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<MergedMeshDebug>("MergedMeshDebug", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Vegetation")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Level", 0x9aeacc13))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(0, &MergedMeshDebug::m_stressInfo, "Sector Info", "Shows sector outlines")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MergedMeshDebug::OnChangedValues)
                    ->DataElement(0, &MergedMeshDebug::m_printState, "Budget Info", "Shows the GFX budgets for the merged meshes")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MergedMeshDebug::OnChangedValues)
                    ->DataElement(0, &MergedMeshDebug::m_instanceLines, "Instance Lines", "Shows lines where each instance has been created")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MergedMeshDebug::OnChangedValues)
                    ->DataElement(0, &MergedMeshDebug::m_instanceAABBs, "Instance AABBs", "Draw instances AABBs using different colors for LODs")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MergedMeshDebug::OnChangedValues)
                    ->DataElement(0, &MergedMeshDebug::m_spinesFilter, "Spines Filter", "Draw spines filter")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MergedMeshDebug::OnChangedValues)
                    ->DataElement(0, &MergedMeshDebug::m_calculatedWind, "Calculated Wind", "Show the calculated wind")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MergedMeshDebug::OnChangedValues)
                    ->DataElement(0, &MergedMeshDebug::m_influencingColliders, "Influencing Colliders", "Draw colliders of objects influencing the merged meshes")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MergedMeshDebug::OnChangedValues)
                    ->DataElement(0, &MergedMeshDebug::m_spinesAsLines, "Spines As Lines", "Draw spines as lines")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MergedMeshDebug::OnChangedValues)
                    ->DataElement(0, &MergedMeshDebug::m_simulatedSpines, "Simulated Spines", "Draw simulated spines")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MergedMeshDebug::OnChangedValues)
                    ->DataElement(0, &MergedMeshDebug::m_spinesLOD, "Spines LOD", "Draw spines with LOD info (red/blue)")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MergedMeshDebug::OnChangedValues)
                    ;
            }
        }
    }
    
    EditorDebugComponent::MergedMeshDebug::~MergedMeshDebug()
    {
        auto* system = GetISystem();
        if (!system)
        {
            return;
        }

        auto* console = system->GetIConsole();
        if (console)
        {
            auto* cvar = console->GetCVar("e_MergedMeshesDebug");
            if (cvar)
            {
                cvar->Set(m_storedMergedMeshDebugValue);
                m_storedMergedMeshDebugValue = 0;
            }
        }
    }

    void EditorDebugComponent::MergedMeshDebug::StoreCurrentDebugValue()
    {
        auto* system = GetISystem();
        if (!system)
        {
            return;
        }

        auto* console = system->GetIConsole();
        if (console)
        {
            if (auto* cvar = console->GetCVar("e_MergedMeshesDebug"))
            {
                m_storedMergedMeshDebugValue = cvar->GetIVal();
            }
        }
    }

    void EditorDebugComponent::MergedMeshDebug::OnChangedValues()
    {
        auto* system = GetISystem();
        if (!system)
        {
            return;
        }

        auto* console = system->GetIConsole();
        if (console)
        {
            int mergedMeshesDebug = 0;
            mergedMeshesDebug |= m_stressInfo ? static_cast<int>(MergedMeshDebugFlags::StressInfo) : 0;
            mergedMeshesDebug |= m_printState ? static_cast<int>(MergedMeshDebugFlags::PrintState) : 0;
            mergedMeshesDebug |= m_instanceLines ? static_cast<int>(MergedMeshDebugFlags::InstanceLines) : 0;
            mergedMeshesDebug |= m_instanceAABBs ? static_cast<int>(MergedMeshDebugFlags::InstanceAABBs) : 0;
            mergedMeshesDebug |= m_spinesFilter ? static_cast<int>(MergedMeshDebugFlags::SpinesFilter) : 0;
            mergedMeshesDebug |= m_calculatedWind ? static_cast<int>(MergedMeshDebugFlags::CalculatedWind) : 0;
            mergedMeshesDebug |= m_influencingColliders ? static_cast<int>(MergedMeshDebugFlags::InfluencingColliders) : 0;
            mergedMeshesDebug |= m_spinesAsLines ? static_cast<int>(MergedMeshDebugFlags::SpinesAsLines) : 0;
            mergedMeshesDebug |= m_simulatedSpines ? static_cast<int>(MergedMeshDebugFlags::SimulatedSpines) : 0;
            mergedMeshesDebug |= m_spinesLOD ? static_cast<int>(MergedMeshDebugFlags::SpinesLOD) : 0;

            if (auto* cvar = console->GetCVar("e_MergedMeshesDebug"))
            {
                cvar->Set(mergedMeshesDebug);
            }
        }
    }

    namespace EditorDebugComponentVersionUtility
    {
        bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            EditorVegetationComponentBaseVersionConverter<DebugComponent, DebugConfig>(context, classElement);

            if (classElement.GetVersion() < 2)
            {
                classElement.RemoveElementByName(AZ_CRC("FilerTypeLevel", 0x246c4e16));
                classElement.RemoveElementByName(AZ_CRC("SortType", 0xdd2117e6));
            }
            return true;
        }
    }
     
    void EditorDebugComponent::Reflect(AZ::ReflectContext* context)
    {
        MergedMeshDebug::Reflect(context);
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorDebugComponent, BaseClassType>()
                ->Version(2, &EditorDebugComponentVersionUtility::VersionConverter)
                ->Field("MergedMeshDebug", &EditorDebugComponent::m_mergedMeshDebug)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<EditorDebugComponent>(
                    s_componentName, s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Level", 0x9aeacc13))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugComponent::OnDumpDataToFile)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Dump Performance Log")
                    ->DataElement(0, &EditorDebugComponent::m_mergedMeshDebug, "Merged Mesh Debugging", "")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugComponent::OnClearReport)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Clear Performance Log")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugComponent::OnRefreshAllAreas)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Refresh All Areas")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugComponent::OnClearAllAreas)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Clear All Areas")
                    ;
            }
        }
    }

    void EditorDebugComponent::Activate()
    {
        BaseClassType::Activate();
        m_mergedMeshDebug.StoreCurrentDebugValue();
        m_mergedMeshDebug.OnChangedValues();
    }

    void EditorDebugComponent::OnDumpDataToFile()
    {
        m_component.ExportCurrentReport();
    }

    void EditorDebugComponent::OnClearReport()
    {
        m_component.ClearPerformanceReport();
    }

    void EditorDebugComponent::OnRefreshAllAreas()
    {
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::RefreshAllAreas);
    }

    void EditorDebugComponent::OnClearAllAreas()
    {
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::ClearAllAreas);
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::RefreshAllAreas);
    }
}