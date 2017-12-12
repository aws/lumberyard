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

#include "Core.h"
#include "Attributes.h"
#include "Contract.h"

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/ObjectStream.h>

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvas
{
    class Node;
    class Graph;
    class SystemComponent;

    ////////////////////////////////////////////////////////////////
    // SystemRequests 
    ////////////////////////////////////////////////////////////////
    class SystemRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Create a graph and attach it to the supplied Entity
        virtual void CreateGraphOnEntity(AZ::Entity*) = 0;

        //! Create a graph, a pointer to the graph/
        //! The Init() function is not called on the graph to remapping of Entity Id's to still work
        virtual ScriptCanvas::Graph* MakeGraph() = 0;
        virtual AZ::EntityId FindGraphId(AZ::Entity* /*graphEntity*/)
        {
            return AZ::EntityId();
        }

        virtual ScriptCanvas::Node* GetNode(const AZ::EntityId&, const AZ::Uuid&) = 0;

        virtual ScriptCanvas::SystemComponent* ModSystemComponent() = 0;
     
        //! Given the ClassData for a type create a Script Canvas Node Component on the supplied entity
        virtual Node* CreateNodeOnEntity(const AZ::EntityId& entityId, AZ::EntityId graphId, const AZ::Uuid& nodeType) = 0;

        template <typename NodeType>
        NodeType* GetNode(const AZ::EntityId& nodeId)
        {
            AZ::Entity* nodeEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
            if (nodeEntity)
            {
                return nodeEntity->FindComponent<NodeType>();
            }

            return nullptr;
        }
    };

    using SystemRequestBus = AZ::EBus<SystemRequests>;
}