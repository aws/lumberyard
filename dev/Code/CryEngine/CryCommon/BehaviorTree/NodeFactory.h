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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_BEHAVIORTREE_NODEFACTORY_H
#define CRYINCLUDE_CRYCOMMON_BEHAVIORTREE_NODEFACTORY_H
#pragma once

#include "Node.h"
#include "VectorMap.h"
#include "IBehaviorTree.h"
#include "LegacyAllocator.h"


namespace BehaviorTree
{
    class NodeFactory
        : public INodeFactory
    {
    public:
        NodeFactory()
            : m_nextNodeID(0)
        {
        }

        virtual INodePtr CreateNodeOfType(const char* typeName)
        {
            INodePtr node;

            NodeCreators::iterator nodeCreatorIt = m_nodeCreators.find(stack_string(typeName));
            if (nodeCreatorIt != m_nodeCreators.end())
            {
                INodeCreator* creator = nodeCreatorIt->second;
                node = creator->Create();

                static_cast<Node*>(node.get())->m_id = m_nextNodeID++;
                static_cast<Node*>(node.get())->SetCreator(creator);
            }

            if (!node)
            {
                gEnv->pLog->LogError("Failed to create behavior tree node of type '%s'.", typeName);
            }

            return node;
        }

        virtual INodePtr CreateNodeFromXml(const XmlNodeRef& xml, const LoadContext& context)
        {
            INodePtr node = context.nodeFactory.CreateNodeOfType(xml->getTag());

            if (node)
            {
                #ifdef DEBUG_MODULAR_BEHAVIOR_TREE
                static_cast<Node*>(node.get())->SetXmlLine(xml->getLine());
                #endif

                if (node->LoadFromXml(xml, context) == LoadSuccess)
                {
                    return node;
                }
            }

            return INodePtr();
        }

        virtual void RegisterNodeCreator(INodeCreator* nodeCreator)
        {
            nodeCreator->SetNodeFactory(this);
            m_nodeCreators.insert(std::make_pair(stack_string(nodeCreator->GetTypeName()), nodeCreator));

            #ifdef USING_BEHAVIOR_TREE_DEBUG_MEMORY_USAGE
            gEnv->pLog->Log("Modular behavior tree node '%s' has a class size of %" PRISIZE_T " bytes.", nodeCreator->GetTypeName(), nodeCreator->GetNodeClassSize());
            #endif
        }

        virtual void TrimNodeCreators()
        {
            NodeCreators::const_iterator it = m_nodeCreators.begin();
            NodeCreators::const_iterator end = m_nodeCreators.end();

            for (; it != end; ++it)
            {
                INodeCreator* creator = it->second;
                creator->Trim();
            }
        }

        virtual size_t GetSizeOfImmutableDataForAllAllocatedNodes() const override
        {
            size_t total = 0;

            NodeCreators::const_iterator it = m_nodeCreators.begin();
            NodeCreators::const_iterator end = m_nodeCreators.end();

            for (; it != end; ++it)
            {
                INodeCreator* creator = it->second;
                total += creator->GetSizeOfImmutableDataForAllAllocatedNodes();
            }

            return total;
        }

        virtual size_t GetSizeOfRuntimeDataForAllAllocatedNodes() const override
        {
            size_t total = 0;

            NodeCreators::const_iterator it = m_nodeCreators.begin();
            NodeCreators::const_iterator end = m_nodeCreators.end();

            for (; it != end; ++it)
            {
                INodeCreator* creator = it->second;
                total += creator->GetSizeOfRuntimeDataForAllAllocatedNodes();
            }

            return total;
        }

        virtual void* AllocateRuntimeDataMemory(const size_t size) override
        {
            return azmalloc(size, sizeof(void*), AZ::LegacyAllocator);
        }

        virtual void FreeRuntimeDataMemory(void* pointer) override
        {
            assert(azallocsize(pointer, AZ::LegacyAllocator) != 0);
            if (azallocsize(pointer, AZ::LegacyAllocator) != 0)
            {
                azfree(pointer, AZ::LegacyAllocator);
            }
        }

        // This will be called while loading a level or jumping into game
        // in the editor. The memory will remain until the level is unloaded
        // or we exit the game in the editor.
        virtual void* AllocateNodeMemory(const size_t size)
        {
            return azcalloc(size, 0, AZ::LegacyAllocator);
        }

        // This will be called while unloading a level or exiting game
        // in the editor.
        virtual void FreeNodeMemory(void* pointer)
        {
            azfree(pointer, AZ::LegacyAllocator);
        }

    private:
        typedef VectorMap<stack_string, INodeCreator*> NodeCreators;

        NodeCreators m_nodeCreators;
        NodeID m_nextNodeID;
    };
}

#endif // CRYINCLUDE_CRYCOMMON_BEHAVIORTREE_NODEFACTORY_H
