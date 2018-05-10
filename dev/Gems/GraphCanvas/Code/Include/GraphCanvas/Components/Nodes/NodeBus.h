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
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/vector.h>

#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>
#include <GraphCanvas/Types/TranslationTypes.h>

namespace AZ
{
    class any;
}

namespace GraphCanvas
{
    class Scene;

    //! NodeRequests
    //! Requests that get or set the properties of a node.
    class NodeRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Set the tooltip for the node, which will display when the mouse is over the node but not a child item.
        virtual void SetTooltip(const AZStd::string&) = 0;

        //! Set the translation keyed tooltip for the node, which will display when the mouse is over the node but not a child item.
        virtual void SetTranslationKeyedTooltip(const TranslationKeyedString&) = 0;

        //! Get the tooltip that is currently set for the node.
        virtual const AZStd::string& GetTooltip() const = 0;

        //! Sets whether or not the given node will display in the outliner.
        virtual void SetShowInOutliner(bool showInOutliner) = 0;

        //! Get whether to show this node in the outliner or not
        virtual bool ShowInOutliner() const = 0;

        //! Add a slot entity to the node.
        //! The node will manage the slot entity and its entity life-cycle will be linked to that of the node. If the
        //! slot must outlive the node, then it will need to be removed before the node is destroyed.
        virtual void AddSlot(const AZ::EntityId&) = 0;
        //! Remove a slot from the node.
        virtual void RemoveSlot(const AZ::EntityId&) = 0;

        //! Obtain a collection of the entity IDs of the slots owned by a node.
        virtual AZStd::vector<AZ::EntityId> GetSlotIds() const = 0;

        //! Get user data from this node
        virtual AZStd::any* GetUserData() = 0;

        //! Returns whether or not the Node is currently wrapped.
        virtual bool IsWrapped() const = 0;

        virtual void SetWrappingNode(const AZ::EntityId& wrappingNode) = 0;

        virtual AZ::EntityId GetWrappingNode() const = 0;
    };

    using NodeRequestBus = AZ::EBus<NodeRequests>;

    //! NodeNotifications
    //! Notifications about changes to the state of nodes
    class NodeNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Emitted when the node is added to a scene
        virtual void OnAddedToScene(const AZ::EntityId& sceneId) {};

        //! Emitted when the node is removed from a scene
        virtual void OnRemovedFromScene(const AZ::EntityId& sceneId) {};

        //! Emitted when the name of a node changes.
        virtual void OnNameChanged(const AZStd::string&) {}
        //! Emitted when the description of the node is changed.
        virtual void OnDescriptionChanged(const AZStd::string&) {}
        //! When the tooltip of the node is changed, this is emitted.
        virtual void OnTooltipChanged(const AZStd::string&) {}

        //! The addition of a slot to the node causes the emission of this event.
        //! # Parameters
        //! 1. The entity ID of the slot that was added.
        virtual void OnSlotAdded(const AZ::EntityId&) {}

        //! The removal of a slot to the node causes the emission of this event.
        //! # Parameters
        //! 1. The entity ID of the slot that was removed.
        virtual void OnSlotRemoved(const AZ::EntityId&) {}

        virtual void OnNodeActivated() {};

        virtual void OnNodeAboutToSerialize(GraphSerialization&) {}
        virtual void OnNodeDeserialized(const AZ::EntityId& graphId, const GraphSerialization&) {}

        virtual void OnNodeWrapped(const AZ::EntityId& wrappingNode) {}
        virtual void OnNodeUnwrapped(const AZ::EntityId& wrappingNode) {}
    };

    using NodeNotificationBus = AZ::EBus<NodeNotifications>;
}