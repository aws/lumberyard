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
#include "precompiled.h"

#include <AzCore/Component/ComponentApplicationBus.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/SceneBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/VariableNodeDescriptorComponent.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/PropertyGridBus.h>

namespace ScriptCanvasEditor
{
    ////////////////////////////////////
    // VariableNodeDescriptorComponent
    ////////////////////////////////////

    void VariableNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<VariableNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ;
        }
    }

    VariableNodeDescriptorComponent::VariableNodeDescriptorComponent()
    {
    }

    void VariableNodeDescriptorComponent::Activate()
    {
        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void VariableNodeDescriptorComponent::Deactivate()
    {
        ScriptCanvas::VariableNodeNotificationBus::Handler::BusDisconnect();
    }

    void VariableNodeDescriptorComponent::OnVariableRenamed(AZStd::string_view variableName)
    {
        UpdateTitle(variableName);
    }

    void VariableNodeDescriptorComponent::OnVariableRemoved()
    {
        AZ_Error("ScriptCanvas", false, "Removing a variable from node that is still in use. Deleting node");
        AZStd::unordered_set< AZ::EntityId > deleteIds = { GetEntityId() };

        AZ::EntityId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        // After this call the component is no longer valid.
        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, deleteIds);
    }

    void VariableNodeDescriptorComponent::OnVariableIdChanged(const ScriptCanvas::VariableId& /*oldVariableId*/, const ScriptCanvas::VariableId& newVariableId)
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
        VariableGraphMemberRefCountRequestBus::Handler::BusDisconnect();

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(newVariableId);
        VariableGraphMemberRefCountRequestBus::Handler::BusConnect(newVariableId);

        ScriptCanvas::Data::Type scriptCanvasType;
        ScriptCanvas::VariableRequestBus::EventResult(scriptCanvasType, newVariableId, &ScriptCanvas::VariableRequests::GetType);

        AZStd::string_view typeName = TranslationHelper::GetSafeTypeName(scriptCanvasType);
        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetSubTitle, typeName);

        AZ::Uuid dataType = ScriptCanvas::Data::ToAZType(scriptCanvasType);
        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetDataPaletteOverride, dataType);

        AZStd::string_view variableName;
        ScriptCanvas::VariableRequestBus::EventResult(variableName, newVariableId, &ScriptCanvas::VariableRequests::GetName);
        UpdateTitle(variableName);

        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RebuildPropertyGrid);
    }

    void VariableNodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        OnVariableIdChanged({}, GetVariableId());

        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        AZ::EntityId* scNodeId = AZStd::any_cast<AZ::EntityId>(userData);
        if (scNodeId)
        {
            ScriptCanvas::VariableNodeNotificationBus::Handler::BusConnect(*scNodeId);
        }
    }

    void VariableNodeDescriptorComponent::OnNodeAboutToSerialize(GraphCanvas::GraphSerialization& graphSerialization)
    {
        auto& userDataMapRef = graphSerialization.GetUserDataMapRef();
        
        auto mapIter = userDataMapRef.find(ScriptCanvas::CopiedVariableData::k_variableKey);

        ScriptCanvas::CopiedVariableData::VariableMapping* variableConfigurations = nullptr;

        if (mapIter == userDataMapRef.end())
        {
            ScriptCanvas::CopiedVariableData variableData;
            auto insertResult = userDataMapRef.emplace(ScriptCanvas::CopiedVariableData::k_variableKey, variableData);

            ScriptCanvas::CopiedVariableData* copiedVariableData = AZStd::any_cast<ScriptCanvas::CopiedVariableData>(&insertResult.first->second);
            variableConfigurations = (&copiedVariableData->m_variableMapping);
        }
        else
        {
            ScriptCanvas::CopiedVariableData* copiedVariableData = AZStd::any_cast<ScriptCanvas::CopiedVariableData>(&mapIter->second);
            variableConfigurations = (&copiedVariableData->m_variableMapping);
        }

        ScriptCanvas::VariableId variableId = GetVariableId();

        if (variableConfigurations->find(variableId) == variableConfigurations->end())
        {
            AZ::EntityId scriptCanvasGraphId;
            GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetActiveScriptCanvasGraphId);

            ScriptCanvas::VariableNameValuePair* configuration = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(configuration, scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);            

            if (configuration)
            {
                variableConfigurations->emplace(GetVariableId(), (*configuration));
            }
        }
    }

    void VariableNodeDescriptorComponent::OnNodeDeserialized(const AZ::EntityId& graphCanvasGraphId, const GraphCanvas::GraphSerialization& graphSerialization)
    {
        ScriptCanvas::VariableNameValuePair* variableData = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableData, graphCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, GetVariableId());

        // If the variable is null. We need to create it from our copied data.
        if (variableData == nullptr)
        {
            const auto& userDataMapRef = graphSerialization.GetUserDataMapRef();

            auto mapIter = userDataMapRef.find(ScriptCanvas::CopiedVariableData::k_variableKey);

            if (mapIter != userDataMapRef.end())
            {
                const ScriptCanvas::CopiedVariableData* copiedVariableData = AZStd::any_cast<ScriptCanvas::CopiedVariableData>(&mapIter->second);
                const ScriptCanvas::CopiedVariableData::VariableMapping* mapping = (&copiedVariableData->m_variableMapping);

                ScriptCanvas::VariableId originalVariable = GetVariableId();
                auto variableIter = mapping->find(originalVariable);

                if (variableIter != mapping->end())
                {
                    AZ::EntityId scriptCanvasGraphId;
                    GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasGraphId, graphCanvasGraphId);
                    
                    const ScriptCanvas::VariableNameValuePair& variableConfiguration = variableIter->second;

                    AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> remapVariableOutcome = AZ::Failure(AZStd::string());
                    ScriptCanvas::GraphVariableManagerRequestBus::EventResult(remapVariableOutcome, scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::RemapVariable, variableConfiguration);

                    if (remapVariableOutcome)
                    {
                        SetVariableId(remapVariableOutcome.GetValue());
                    }
                }
            }
        }
    }

    ScriptCanvas::VariableId VariableNodeDescriptorComponent::GetVariableId() const
    {
        ScriptCanvas::VariableId variableId;

        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        AZ::EntityId* scNodeId = AZStd::any_cast<AZ::EntityId>(userData);
        if (scNodeId)
        {
            ScriptCanvas::VariableNodeRequestBus::EventResult(variableId, *scNodeId, &ScriptCanvas::VariableNodeRequests::GetId);
        }

        return variableId;
    }

    AZ::EntityId VariableNodeDescriptorComponent::GetGraphMemberId() const
    {
        return GetEntityId();
    }

    void VariableNodeDescriptorComponent::UpdateTitle(AZStd::string_view variableName)
    {
        AZ_Error("ScriptCanvas", false, "Should be pure virtual function, but Pure Virtual functions on components are disallowed.");
    }

    void VariableNodeDescriptorComponent::SetVariableId(const ScriptCanvas::VariableId& variableId)
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        AZ::EntityId* scNodeId = AZStd::any_cast<AZ::EntityId>(userData);
        if (scNodeId)
        {
            ScriptCanvas::VariableNodeRequestBus::Event(*scNodeId, &ScriptCanvas::VariableNodeRequests::SetId, variableId);
        }
    }
}