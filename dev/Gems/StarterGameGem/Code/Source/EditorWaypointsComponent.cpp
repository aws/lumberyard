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

#include "StarterGameGem_precompiled.h"
#include "EditorWaypointsComponent.h"

#include "WaypointsComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>

#include <IRenderAuxGeom.h>

namespace StarterGameGem
{
    namespace ClassConverters
    {
        static bool DeprecatePreEditorWaypointsComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void EditorWaypointsComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorWaypointsComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorWaypointsComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->ClassDeprecate(
                "WaypointsComponent",
                "{3259A366-D177-4B5B-B047-2DD3CE93F984}",
                &ClassConverters::DeprecatePreEditorWaypointsComponent
            );

            serializeContext->Class<EditorWaypointsComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Config", &EditorWaypointsComponent::m_config)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorWaypointsComponent>("Waypoints", "Contains a list of waypoints")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "AI")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->DataElement(0, &EditorWaypointsComponent::m_config, "Config", "Waypoints Configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly"))
                ;
            }
        }
    }

    void EditorWaypointsComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto component = gameEntity->CreateComponent<WaypointsComponent>();
        if (component)
        {
            component->m_config = m_config;
        }
    }

    void EditorWaypointsComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        // Don't draw the waypoints if the CVar either doesn't exist, is set to 0, or the number
        // of waypoints is less than two.
        static ICVar* const cvarDrawWaypoints = gEnv->pConsole->GetCVar("ai_debugDrawWaypoints");
        int noOfValidWaypoints = m_config.NumberOfValidWaypoints();
        if (!cvarDrawWaypoints)
        {
            return;
        }
        else if (cvarDrawWaypoints->GetFVal() == 0 || noOfValidWaypoints < 2)
        {
            return;
        }

        debugDisplay.PushMatrix(AZ::Transform::CreateIdentity());

        AZ::Vector3 raised = AZ::Vector3(0.0f, 0.0, cvarDrawWaypoints->GetFVal());
        AZ::Vector3* points = new AZ::Vector3[noOfValidWaypoints];
        for (size_t i = 0; i < m_config.m_waypoints.size(); ++i)
        {
            if (!m_config.m_waypoints[i].IsValid())
                continue;

            AZ::Transform tm = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(tm, m_config.m_waypoints[i], &AZ::TransformInterface::GetWorldTM);
            points[i] = tm.GetTranslation() + raised;
        }

        debugDisplay.SetColor(s_waypointsDebugLineColourVec);
        debugDisplay.DrawPolyLine(points, noOfValidWaypoints);

        debugDisplay.PopMatrix();
    }

    namespace ClassConverters
    {
        static bool DeprecatePreEditorWaypointsComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="WaypointsComponent" field="m_template" version="1" type="{3259A366-D177-4B5B-B047-2DD3CE93F984}">
                <Class name="bool" field="IsSentry" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                <Class name="bool" field="IsLazySentry" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                <Class name="AZStd::vector" field="Waypoints" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}">
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="17133002231682569749" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="17133002240272504341" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="17429177962414724816" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="17429177966709692112" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="17429177962414724816" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="17133002240272504341" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                </Class>
                <Class name="unsigned int" field="CurrentWaypoint" value="3452816845" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
            </Class>

            New:
            <Class name="EditorWaypointsComponent" field="m_template" version="1" type="{C15A4DA3-7CDE-48B2-935C-E8F5EC93E045}">
                <Class name="WaypointsConfiguration" field="Config" version="1" type="{36B50649-8905-4D40-906E-8A916A2B4EFC}">
                    <Class name="bool" field="IsSentry" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                    <Class name="bool" field="IsLazySentry" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                    <Class name="AZStd::vector" field="Waypoints" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}">
                        <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                            <Class name="AZ::u64" field="id" value="17133002231682569749" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                        </Class>
                        <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                            <Class name="AZ::u64" field="id" value="17133002240272504341" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                        </Class>
                        <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                            <Class name="AZ::u64" field="id" value="17429177962414724816" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                        </Class>
                        <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                            <Class name="AZ::u64" field="id" value="17429177966709692112" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                        </Class>
                        <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                            <Class name="AZ::u64" field="id" value="17429177962414724816" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                        </Class>
                        <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                            <Class name="AZ::u64" field="id" value="17133002240272504341" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                        </Class>
                    </Class>
                    <Class name="unsigned int" field="CurrentWaypoint" value="3452816845" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
                </Class>
            </Class>
            */

            // Find all the data in the old version and store it in the new
            // 'WaypointsConfiguration' struct.
            WaypointsConfiguration config;
            int isSentryIndex = classElement.FindElement(AZ_CRC("IsSentry"));
            int isLazySentryIndex = classElement.FindElement(AZ_CRC("IsLazySentry"));
            int currentWaypointIndex = classElement.FindElement(AZ_CRC("CurrentWaypoint"));
            int waypointsIndex = classElement.FindElement(AZ_CRC("Waypoints"));
            if (isSentryIndex != -1)
            {
                classElement.GetSubElement(isSentryIndex).GetData<bool>(config.m_isSentry);
            }
            if (isLazySentryIndex != -1)
            {
                classElement.GetSubElement(isLazySentryIndex).GetData<bool>(config.m_isLazySentry);
            }
            if (currentWaypointIndex != -1)
            {
                classElement.GetSubElement(currentWaypointIndex).GetData<AZ::u32>(config.m_currentWaypoint);
            }
            if (waypointsIndex != -1)
            {
                // This doesn't work... not sure why. To me, it would seem reasonable that if it
                // can serialize it without being given more information than this that it should
                // be able to deserialize it.
                //classElement.GetSubElement(waypointsIndex).GetData<VecOfEntityIds>(config.m_waypoints);

                AZ::SerializeContext::DataElementNode waypointsNode = classElement.GetSubElement(waypointsIndex);
                int waypointsCount = waypointsNode.GetNumSubElements();
                for (int i = 0; i < waypointsCount; ++i)
                {
                    AZ::SerializeContext::DataElementNode entityIdNode = waypointsNode.GetSubElement(i);
                    int idIndex = entityIdNode.FindElement(AZ_CRC("id"));
                    if (idIndex != -1)
                    {
                        AZ::u64 entityId;
                        entityIdNode.GetSubElement(idIndex).GetData<AZ::u64>(entityId);
                        config.m_waypoints.push_back(AZ::EntityId(entityId));
                    }
                }
            }

            // Now supplant the old component with the new format.
            bool result = classElement.Convert(context, "{C15A4DA3-7CDE-48B2-935C-E8F5EC93E045}");  // GUID of the new component
            if (result)
            {
                int configIndex = classElement.AddElement<WaypointsConfiguration>(context, "Config");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<WaypointsConfiguration>(context, config);
                }
            }
            return result;
        }
    }
} // namespace StarterGameGem
