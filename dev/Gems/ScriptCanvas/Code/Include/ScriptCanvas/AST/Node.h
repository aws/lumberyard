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

#include "AST.h"

#include <ScriptCanvas/Grammar/Grammar.h>

namespace ScriptCanvas
{
    class Node;

    namespace AST
    {
        using NodePtr = SmartPtr<Node>;
        using NodePtrConst = SmartPtrConst<Node>;
        using NodeChildren = AZStd::vector<NodePtrConst>;

        struct NodeCtrInfo
        {
            Grammar::eRule m_rule;
            const ScriptCanvas::Node* m_node = nullptr;
            // source code graph, nodeinfo, etc.

            NodeCtrInfo() = default;

            AZ_INLINE NodeCtrInfo(const ScriptCanvas::Node& node)
                : m_rule(Grammar::eRule::Count)
                , m_node(&node)                
            {}

            AZ_INLINE NodeCtrInfo(const NodeCtrInfo& info, Grammar::eRule rule)
                : m_rule(rule)
                , m_node(info.m_node)
            {}
        };

        class Node
        {
        public:
            AZ_RTTI(Node,"{97C2873E-F1DE-4D19-9D6E-9DFCC159CE6D}"); 
            AZ_CLASS_ALLOCATOR(Node, AZ::SystemAllocator, 0);
            
            Node(const NodeCtrInfo& info);
            
            virtual ~Node() = default;

            AZ_INLINE void AddChild(NodePtrConst&& child) { AZ_Assert(child, "no null nodes!"); m_children.emplace_back(child); }
            
            AZ_INLINE NodePtrConst GetChild(int index) const;

            template<typename t_NodeType>
            AZ_INLINE SmartPtrConst<t_NodeType> GetChildAs(int index) const;
            
            AZ_INLINE const NodeChildren& GetChildren() const;
            
            const char* GetName() const;

            AZ_INLINE Grammar::eRule GetRule() const;
                        
            void PrettyPrint() const;

            virtual const char* ToString() const;

            virtual void Visit(Visitor& visitor) const = 0;

        protected:
            AZ_INLINE NodeChildren& ModChildren();
            
            void PrettyPrint(int depth, AZStd::string& output) const;

        private:
            NodeChildren m_children;
            const NodeCtrInfo m_info;

            // intrusive_ptr compatibility begin
            template<typename T>
            friend struct AZStd::IntrusivePtrCountPolicy;
            mutable RefCount m_refCount;
            AZ_FORCE_INLINE void add_ref() const { ++m_refCount; }
            AZ_FORCE_INLINE void release() const { if (--m_refCount == 0) { delete this; } }
            // intrusive_ptr compatibility end
        };

        NodePtrConst Node::GetChild(int index) const 
        { 
            return index < m_children.size() ? m_children[index] : nullptr;
        }

        template<typename t_NodeType>
        SmartPtrConst<t_NodeType> Node::GetChildAs(int index) const
        {
            return azrtti_cast<const t_NodeType*>(GetChild(index));
        }

        const NodeChildren& Node::GetChildren() const
        {
            return m_children;
        }

        Grammar::eRule Node::GetRule() const
        { 
            return m_info.m_rule; 
        }
        
        NodeChildren& Node::ModChildren() 
        { 
            return m_children; 
        }
    }
} // namespace ScriptCanvas