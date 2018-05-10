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

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

namespace ScriptCanvasEditor
{
    class NodeDescriptorComponent
        : public AZ::Component
        , public NodeDescriptorRequestBus::Handler
    {
    public:
        AZ_COMPONENT(NodeDescriptorComponent, "{C775A98E-D64E-457F-8ABA-B34CBAD10905}");
        static void Reflect(AZ::ReflectContext* reflect)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflect))
            {
                serializeContext->Class<NodeDescriptorComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }

        NodeDescriptorComponent()
            : m_nodeDescriptorType(NodeDescriptorType::Unknown)
        {
        }
        
        NodeDescriptorComponent(NodeDescriptorType descriptorType)
            : m_nodeDescriptorType(descriptorType)
        {
        }
        
        ~NodeDescriptorComponent() override = default;

        // NodeDescriptorBus::Handler
        NodeDescriptorType GetType() const override { return m_nodeDescriptorType; }
        ////
        
        void Init()
        {
        }
        
        void Activate()
        {
            NodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
        }
        
        void Deactivate()
        {
            NodeDescriptorRequestBus::Handler::BusDisconnect();
        }
        
    private:
    
        NodeDescriptorType m_nodeDescriptorType;
    };
}