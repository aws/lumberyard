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
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/SystemComponent.h>

namespace ScriptCanvas
{
    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectLibraries(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0)
                // ScriptCanvas avoids a use dependency on the AssetBuilderSDK. Therefore the Crc is used directly to register this component with the Gem builder
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC("AssetBuilder", 0xc739c7d7) }));
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
        RegisterCreatableTypes();
    }

    void SystemComponent::Activate()
    {
        SystemRequestBus::Handler::BusConnect();
        AZ::BehaviorContext* behaviorContext{};
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        if (behaviorContext)
        {
            AZ::BehaviorContextBus::Handler::BusConnect(behaviorContext);
        }
    }

    void SystemComponent::Deactivate()
    {
        AZ::BehaviorContextBus::Handler::BusDisconnect();
        SystemRequestBus::Handler::BusDisconnect();
    }

    void SystemComponent::CreateEngineComponentsOnEntity(AZ::Entity* entity)
    {
        if (entity)
        {
            auto graph = entity->CreateComponent<Graph>();
            entity->CreateComponent<GraphVariableManagerComponent>(graph->GetUniqueId());
        }
    }

    Graph* SystemComponent::CreateGraphOnEntity(AZ::Entity* graphEntity)
    {
        return graphEntity ? graphEntity->CreateComponent<Graph>() : nullptr;
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

    void SystemComponent::RegisterCreatableTypes()
    {
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Serialize Context should not be missing at this point");
        AZ::BehaviorContext* behaviorContext{};
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Behavior Context should not be missing at this point");

        auto dataRegistry = ScriptCanvas::GetDataRegistry();
        for (const auto& classIter : behaviorContext->m_classes)
        {
            const AZ::BehaviorClass* behaviorClass = classIter.second;

            // BehaviorContext classes with the ExcludeFrom attribute with a value of the ExcludeFlags::List is not creatable
            const AZ::u64 exclusionFlags = AZ::Script::Attributes::ExcludeFlags::List;
            auto excludeClassAttributeData = azrtti_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes));
            if (excludeClassAttributeData && (excludeClassAttributeData->Get(nullptr) & exclusionFlags))
            {
                continue;
            }

            bool canCreate = serializeContext->FindClassData(behaviorClass->m_typeId) != nullptr;

            // create able variables must have full memory support
            canCreate = canCreate &&
                (behaviorClass->m_allocate
                    && behaviorClass->m_cloner
                    && behaviorClass->m_mover
                    && behaviorClass->m_destructor
                    && behaviorClass->m_deallocate) &&
                AZStd::none_of(behaviorClass->m_baseClasses.begin(), behaviorClass->m_baseClasses.end(), [](const AZ::TypeId& base) { return azrtti_typeid<AZ::Component>() == base; });

            if (canCreate)
            {
                dataRegistry->RegisterType(behaviorClass->m_typeId);
            }
        }
    }

    void SystemComponent::OnAddClass(const char*, AZ::BehaviorClass* behaviorClass)
    {
        auto dataRegistry = ScriptCanvas::GetDataRegistry();
        if (!dataRegistry)
        {
            return;
        }
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Serialize Context should not be missing at this point");

        // BehaviorContext classes with the ExcludeFrom attribute with a value of the ExcludeFlags::List is not creatable
        const AZ::u64 exclusionFlags = AZ::Script::Attributes::ExcludeFlags::List;
        auto excludeClassAttributeData = azrtti_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes));
        bool canCreate = !excludeClassAttributeData || !(excludeClassAttributeData->Get(nullptr) & exclusionFlags);

        canCreate = canCreate && serializeContext->FindClassData(behaviorClass->m_typeId) != nullptr;

        // create able variables must have full memory support
        canCreate = canCreate &&
            (behaviorClass->m_allocate
                && behaviorClass->m_cloner
                && behaviorClass->m_mover
                && behaviorClass->m_destructor
                && behaviorClass->m_deallocate) &&
            AZStd::none_of(behaviorClass->m_baseClasses.begin(), behaviorClass->m_baseClasses.end(), [](const AZ::TypeId& base) { return azrtti_typeid<AZ::Component>() == base; });

        if (canCreate)
        {

            dataRegistry->RegisterType(behaviorClass->m_typeId);
        }
    }

    void SystemComponent::OnRemoveClass(const char*, AZ::BehaviorClass* behaviorClass)
    {
        // The data registry might not be available when unloading the ScriptCanvas module
        auto dataRegistry = ScriptCanvas::GetDataRegistry();
        if (dataRegistry)
        {
            dataRegistry->UnregisterType(behaviorClass->m_typeId);
        }
    }
}