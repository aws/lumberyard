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
#include "TraitsVisitor.h"

#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Libraries/Core/Start.h>

namespace ScriptCanvas 
{
namespace Grammar
{
    TraitsVisitor::TraitsVisitor()
    {
        ResetTraits();
    }
    
    bool TraitsVisitor::IsPrecededByStatementNode(const Node& node)
    {
        AZStd::vector<Endpoint> connectedEndpoints;
        GraphRequestBus::EventResult(connectedEndpoints, node.GetGraphId(), &GraphRequests::GetConnectedEndpoints, Endpoint{ node.GetEntityId(), node.GetSlotId("In") });
        for (const auto& endpoint : connectedEndpoints)
        {
            AZ::Entity* previousEntity{};
            AZ::ComponentApplicationBus::BroadcastResult(previousEntity, &AZ::ComponentApplicationRequests::FindEntity, endpoint.GetNodeId());
            const auto* previousNode = previousEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(previousEntity) : nullptr;

            if (IsStartNode(*previousNode))
            {
                break;
            }

            TraitsVisitor previousNodeTraits;
            previousNode->Visit(previousNodeTraits);

            if (previousNodeTraits.IsStatement() || IsPrecededByStatementNode(*previousNode))
            {
                return true;
            }
        }

        return false;
    }

    bool TraitsVisitor::IsStartNode(const Node& node)
    {
        return azdynamic_cast<const Nodes::Core::Start*>(&node) != nullptr;
    }

    void TraitsVisitor::ResetTraits()
    {
        m_expectsArguments = false;
        m_isExpression = false;
        m_isFunctionCall = false;
        m_isInitialStatement = false;
        m_isPureData = false;
        m_isStatement = false;
    }
        
    void TraitsVisitor::Visit(const Nodes::Math::Divide& node)
    {
        ResetTraits();
        m_isExpression = true;
    }

    void TraitsVisitor::Visit(const Nodes::Math::Multiply& node)
    {
        ResetTraits();
        m_isExpression = true;
    }

    void TraitsVisitor::Visit(const Nodes::Math::Number& node)
    {
        ResetTraits();
        m_isExpression = true;
    }

    void TraitsVisitor::Visit(const Nodes::Math::Subtract& node)
    {
        ResetTraits();
        m_isExpression = true;
    }

    void TraitsVisitor::Visit(const Nodes::Math::Sum& node)
    {
        ResetTraits();
        m_isExpression = true;
    }

} // namespace Grammar
} // namespace ScriptCanvas