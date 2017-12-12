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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/EntityUtils.h>

#include <Libraries/Libraries.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/GraphAsset.h>
#include <ScriptCanvas/SystemComponent.h>

namespace ScriptCanvas
{
    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectLibraries(context);
        GraphAsset::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SystemComponent>("Script Canvas", "Script Canvas System Component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void SystemComponent::Init()
    {
    }

    void SystemComponent::Activate()
    {
        SystemRequestBus::Handler::BusConnect();
    }

    void SystemComponent::Deactivate()
    {
        SystemRequestBus::Handler::BusDisconnect();
    }

    void SystemComponent::CreateGraphOnEntity(AZ::Entity* graphEntity)
    {
        if (graphEntity)
        {
            graphEntity->CreateComponent<Graph>();
        }
    }

    ScriptCanvas::Graph* SystemComponent::MakeGraph()
    {
        AZ::Entity* graphEntity = aznew AZ::Entity("Script Canvas");
        ScriptCanvas::Graph* graph = graphEntity->CreateComponent<ScriptCanvas::Graph>();
        return graph;
    }

    AZ::EntityId SystemComponent::FindGraphId(AZ::Entity* graphEntity)
    {
        auto* graph = graphEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(graphEntity) : nullptr;
        return graph ? graph->GetUniqueId() : AZ::EntityId();
    }

    ScriptCanvas::Node* SystemComponent::GetNode(const AZ::EntityId& nodeId, const AZ::Uuid& typeId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
        if (entity)
        {
            ScriptCanvas::Node* node = azrtti_cast<ScriptCanvas::Node*>(entity->FindComponent(typeId));
            return node;
        }

        return nullptr;
    }

    ScriptCanvas::Node* SystemComponent::CreateNodeOnEntity(const AZ::EntityId& entityId, AZ::EntityId graphId, const AZ::Uuid& nodeType)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve application serialize context");

        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(nodeType);
        AZ_Assert(classData, "Type %s is not registered in the serialization context", nodeType.ToString<AZStd::string>().data());

        if (classData)
        {
            AZ::Entity* nodeEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            ScriptCanvas::Node* node = reinterpret_cast<ScriptCanvas::Node*>(classData->m_factory->Create(classData->m_name));
            AZ_Assert(node, "ClassData (%s) does not correspond to a supported ScriptCanvas Node", classData->m_name);
            if (node && nodeEntity)
            {
                nodeEntity->AddComponent(node);
            }

            GraphRequestBus::Event(graphId, &GraphRequests::AddNode, nodeEntity->GetId());
            return node;
        }

        return nullptr;
    }

    void SystemComponent::AddOwnedObjectReference(const void* object, BehaviorContextObject* behaviorContextObject)
    {
        LockType lock(m_ownedObjectsByAddressMutex);
        m_ownedObjectsByAddress.emplace(object, behaviorContextObject);
    }

    BehaviorContextObject* SystemComponent::FindOwnedObjectReference(const void* object)
    {
        LockType lock(m_ownedObjectsByAddressMutex);
        auto iter = m_ownedObjectsByAddress.find(object);
        return iter == m_ownedObjectsByAddress.end() ? nullptr : iter->second;
    }

    void SystemComponent::RemoveOwnedObjectReference(const void* object)
    {
        LockType lock(m_ownedObjectsByAddressMutex);
        m_ownedObjectsByAddress.erase(object);
    }
}