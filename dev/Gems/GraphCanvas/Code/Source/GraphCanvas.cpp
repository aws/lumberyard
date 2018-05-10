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
#include <precompiled.h>
#include <GraphCanvas.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/EntityUtils.h>

#include <Components/BookmarkManagerComponent.h>
#include <Components/BookmarkAnchor/BookmarkAnchorComponent.h>
#include <Components/BookmarkAnchor/BookmarkAnchorVisualComponent.h>
#include <Components/GeometryComponent.h>
#include <Components/GridComponent.h>
#include <Components/GridVisualComponent.h>
#include <Components/SceneComponent.h>
#include <Components/SceneMemberComponent.h>
#include <Components/StylingComponent.h>

#include <Components/Connections/ConnectionComponent.h>
#include <Components/Connections/ConnectionVisualComponent.h>
#include <Components/Connections/DataConnections/DataConnectionComponent.h>
#include <Components/Connections/DataConnections/DataConnectionVisualComponent.h>

#include <Components/Nodes/NodeComponent.h>
#include <Components/Nodes/Comment/BlockCommentNodeFrameComponent.h>
#include <Components/Nodes/Comment/BlockCommentNodeLayoutComponent.h>
#include <Components/Nodes/Comment/CommentNodeLayoutComponent.h>
#include <Components/Nodes/General/GeneralNodeLayoutComponent.h>
#include <Components/Nodes/General/GeneralSlotLayoutComponent.h>
#include <Components/Nodes/Wrapper/WrapperNodeLayoutComponent.h>

#include <Components/NodePropertyDisplays/BooleanNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/EntityIdNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/ItemModelNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/NumericNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/ReadOnlyNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/StringNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/VectorNodePropertyDisplay.h>

#include <Components/Slots/Data/DataSlotComponent.h>
#include <Components/Slots/Execution/ExecutionSlotComponent.h>
#include <Components/Slots/Property/PropertySlotComponent.h>

#include <GraphCanvas/Styling/Selector.h>
#include <GraphCanvas/Styling/SelectorImplementations.h>
#include <GraphCanvas/Styling/PseudoElement.h>

#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Types/TranslationTypes.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>

namespace GraphCanvas
{
    bool EntitySaveDataContainerVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 1)
        {
            AZ::Crc32 componentDataId = AZ_CRC("ComponentData", 0xa6acc628);
            AZStd::unordered_map< AZ::Uuid, GraphCanvas::ComponentSaveData* > componentSaveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(componentDataId);

            if (dataNode)
            {
                dataNode->GetDataHierarchy(context, componentSaveData);
            }

            classElement.RemoveElementByName(componentDataId);

            AZStd::unordered_map< AZ::Uuid, GraphCanvas::ComponentSaveData* > remappedComponentSaveData;

            for (const auto& mapPair : componentSaveData)
            {
                auto mapIter = remappedComponentSaveData.find(azrtti_typeid(mapPair.second));

                if (mapIter == remappedComponentSaveData.end())
                {
                    remappedComponentSaveData[azrtti_typeid(mapPair.second)] = mapPair.second;
                }
            }

            classElement.AddElementWithData(context, "ComponentData", remappedComponentSaveData);
        }

        return true;
    }

    void GraphCanvasSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphCanvasSystemComponent, AZ::Component>()
                ->Version(1)
                ;

            // Reflect information for the stripped down saving
            serializeContext->Class<ComponentSaveData>()
                ->Version(1)
                ;

            serializeContext->Class<EntitySaveDataContainer>()
                ->Version(2, &EntitySaveDataContainerVersionConverter)
                ->Field("ComponentData", &EntitySaveDataContainer::m_entityData)
                ;
            ////

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GraphCanvasSystemComponent>(
                    "LmbrCentral", "Provides factory methods for Graph Canvas components")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }

            NodeConfiguration::Reflect(serializeContext);
            Styling::SelectorImplementation::Reflect(serializeContext);
            Styling::Selector::Reflect(serializeContext);
            Styling::BasicSelector::Reflect(serializeContext);
            Styling::DefaultSelector::Reflect(serializeContext);
            Styling::CompoundSelector::Reflect(serializeContext);
            Styling::NestedSelector::Reflect(serializeContext);
            TranslationKeyedString::Reflect(serializeContext);
            Styling::Style::Reflect(serializeContext);

        }

        GraphCanvasTreeModel::Reflect(context);
    }

    void GraphCanvasSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(GraphCanvasRequestsServiceId);
    }

    void GraphCanvasSystemComponent::Activate()
    {
        GraphCanvasRequestBus::Handler::BusConnect();
        Styling::PseudoElementFactoryRequestBus::Handler::BusConnect();
    }

    void GraphCanvasSystemComponent::Deactivate()
    {
        Styling::PseudoElementFactoryRequestBus::Handler::BusDisconnect();
        GraphCanvasRequestBus::Handler::BusDisconnect();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateBookmarkAnchor() const
    {
        AZ::Entity* entity = aznew AZ::Entity("BookmarkAnchor");
        entity->CreateComponent<BookmarkAnchorComponent>();
        entity->CreateComponent<BookmarkAnchorVisualComponent>();
        entity->CreateComponent<SceneMemberComponent>();
        entity->CreateComponent<GeometryComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::BookmarkAnchor, AZ::EntityId());

        return entity;
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateScene() const
    {
        // create a new empty canvas, give it a name to avoid serialization generating one based on
        // the ID (which in some cases caused diffs to fail in the editor)
        AZ::Entity* entity = aznew AZ::Entity("GraphCanvasScene");
        entity->CreateComponent<SceneComponent>();
        entity->CreateComponent<BookmarkManagerComponent>();
        return entity;
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateCoreNode() const
    {
        return NodeComponent::CreateCoreNodeEntity();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateGeneralNode(const char* nodeType) const
    {
        return GeneralNodeLayoutComponent::CreateGeneralNodeEntity(nodeType);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateCommentNode() const
    {
        return CommentNodeLayoutComponent::CreateCommentNodeEntity();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateBlockCommentNode() const
    {
        return BlockCommentNodeLayoutComponent::CreateBlockCommentNodeEntity();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateWrapperNode(const char* nodeType) const
    {
        return WrapperNodeLayoutComponent::CreateWrapperNodeEntity(nodeType);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateDataSlot(const AZ::EntityId& nodeId, const AZ::Uuid& typeId, const SlotConfiguration& slotConfiguration) const
    {
        const bool isReference = false;
        return DataSlotComponent::CreateDataSlot(nodeId, typeId, isReference, slotConfiguration);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateVariableReferenceSlot(const AZ::EntityId& nodeId, const AZ::Uuid& typeId, const SlotConfiguration& slotConfiguration) const
    {
        const bool isReference = true;
        return DataSlotComponent::CreateDataSlot(nodeId, typeId, isReference, slotConfiguration);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateVariableSourceSlot(const AZ::EntityId& nodeId, const AZ::Uuid& typeId, const SlotConfiguration& slotConfiguration) const
    {
        // For now, to help deal with copying/pasting. Going to assume that the node id is the variable id.
        // This does put a limit on one variable slot per node though.
        // Otherwise there is no way of distinguishing them.
        //
        // Is fixable(entityId's get remapped on copy/paste), but not until there is a use case.
        // for multiple variables from a single node.
        return DataSlotComponent::CreateVariableSlot(nodeId, typeId, nodeId, slotConfiguration);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateBooleanNodePropertyDisplay(BooleanDataInterface* dataInterface) const
    {
        return aznew BooleanNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateNumericNodePropertyDisplay(NumericDataInterface* dataInterface) const
    {
        return aznew NumericNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateEntityIdNodePropertyDisplay(EntityIdDataInterface* dataInterface) const
    {
        return aznew EntityIdNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateItemModelNodePropertyDisplay(ItemModelDataInterface* dataInterface) const
    {
        return aznew ItemModelNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateReadOnlyNodePropertyDisplay(ReadOnlyDataInterface* dataInterface) const
    {
        return aznew ReadOnlyNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateStringNodePropertyDisplay(StringDataInterface* dataInterface) const
    {
        return aznew StringNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateVectorNodePropertyDisplay(VectorDataInterface* dataInterface) const
    {
        return aznew VectorNodePropertyDisplay(dataInterface);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateExecutionSlot(const AZ::EntityId& nodeId, const SlotConfiguration& configuration) const
    {
        return ExecutionSlotComponent::CreateExecutionSlot(nodeId, configuration);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreatePropertySlot(const AZ::EntityId& nodeId, const AZ::Crc32& propertyId, const SlotConfiguration& configuration) const
    {
        return PropertySlotComponent::CreatePropertySlot(nodeId, propertyId, configuration);
    }

    AZ::EntityId GraphCanvasSystemComponent::CreateStyleEntity(const AZStd::string& style) const
    {
        return StylingComponent::CreateStyleEntity(style);
    }

    AZ::EntityId GraphCanvasSystemComponent::CreateVirtualChild(const AZ::EntityId& real, const AZStd::string& virtualChildElement) const
    {
        return Styling::VirtualChildElement::Create(real, virtualChildElement);
    }
}
