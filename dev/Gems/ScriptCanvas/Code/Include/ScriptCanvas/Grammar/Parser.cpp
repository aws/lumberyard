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
#include <ScriptCanvas/Grammar/Parser.h>

#include <ScriptCanvas/AST/Block.h>
#include <ScriptCanvas/AST/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Libraries/Core/Core.h>
#include <ScriptCanvas/Libraries/Logic/Logic.h>
#include <ScriptCanvas/Libraries/Math/Math.h>

#include "TraitsVisitor.h"
#include "ParserVisitor.h"

namespace ScriptCanvas 
{
    namespace Grammar
    {
        NodePtrConstList FindInitialStatements(const NodeIdList& nodes)
        {
            NodePtrConstList initialStatements;
            Grammar::TraitsVisitor visitor;

            for (const auto& id : nodes)
            {
                AZ::Entity* previousEntity{};
                AZ::ComponentApplicationBus::BroadcastResult(previousEntity, &AZ::ComponentApplicationRequests::FindEntity, id);
                if (const auto* node = previousEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(previousEntity) : nullptr)
                {
                    node->Visit(visitor);
                    
                    if (visitor.IsInitialStatement())
                    {
                        initialStatements.push_back(node);
                    }
                }
            }

            return initialStatements;
        }

        const Node* FindNextStatement(const Node& node)
        {
            const Node* executableNode = &node;
            Grammar::TraitsVisitor visitor;

            while (executableNode = executableNode->GetNextExecutableNode())
            {
                executableNode->Visit(visitor);
                
                if (visitor.IsStatement())
                {
                    return executableNode;
                }
            }

            return nullptr;
        }

        Parser::Parser(const Graph& graph)
            : m_visitor(*this)
        {
            m_executionIterator.Begin(graph);
            m_executionIterator.PrettyPrint();
        }

        void Parser::AddError(ParseError&& error)
        {
            m_errors.emplace_back(error);
        }

        void Parser::AddStatement(AST::SmartPtrConst<AST::Statement>&& statement)
        {
            m_currentBlock->AddChild(std::move(statement));
        }

        AST::SmartPtrConst<AST::Block> Parser::Parse()
        {
            if (IsValid())
            {
                InitializeGraphBlock();
                
                while (const Node* token = GetToken())
                {
                    token->Visit(m_visitor);
                }
            }
            else
            {
                AZ_Warning("Script Dragon", false, "No effective statements; this script does nothing.");
            }

            m_root->PrettyPrint();

            return m_root;
        }

        AST::SmartPtrConst<AST::BinaryOperation> Parser::CreateBinaryOperator(const Node& node, Grammar::eRule operatorType, AST::SmartPtrConst<AST::Expression>&& lhs, AST::SmartPtrConst<AST::Expression>&& rhs)
        {
            switch (operatorType)
            {
            case eRule::AddOperation:
                return aznew AST::AddOperation(AST::NodeCtrInfo(node), std::move(lhs), std::move(rhs));

            case eRule::MultiplyOperation:
                return aznew AST::MultiplyOperation(AST::NodeCtrInfo(node), std::move(lhs), std::move(rhs));

            default:
                AddError(ParseError("unsupported operator encountered"));
            }

            return nullptr;
        }

        void Parser::ConsumeToken()
        {
            if (const Node* sourceNode = GetToken())
            {
                m_consumedSourceNodes.insert(sourceNode->GetEntityId());
                ++m_executionIterator;
            }
        }

        const Node* Parser::GetToken() const
        {
            return m_executionIterator ? m_executionIterator.GetCurrentToken() : nullptr;
        }

        AST::NodePtr Parser::GetTree() const
        {
            return m_root;
        }

        void Parser::InitializeGraphBlock()
        {
            SetRoot(AST::SmartPtr<AST::Block>(aznew AST::Block(AST::NodeCtrInfo())));
            m_currentBlock = m_root;
        }

        bool Parser::IsValid() const
        {
            return bool(m_executionIterator);
        }

        AST::SmartPtrConst<AST::BinaryOperation> Parser::ParseBinaryOperation(const Node& operatorNode, Grammar::eRule operatorType)
        {
            ExpressionScoper expressionScoper(*this);
            ConsumeToken();
            
            if (auto lhsNode = GetToken())
            {
                if (auto lhs = ParseExpression(*lhsNode))
                {
                    if (auto rhsNode = GetToken())
                    {
                        if (auto rhs = ParseExpression(*rhsNode))
                        {
                            return CreateBinaryOperator(operatorNode, operatorType, std::move(lhs), std::move(rhs));
                        }
                    }
                }
            }

            return nullptr;
        }

        AST::SmartPtrConst<AST::Expression> Parser::ParseExpression(const Node& expressionNode)
        {
            ExpressionScoper expressionScoper(*this);
                        
            TraitsVisitor traits;
            expressionNode.Visit(traits);

            if (traits.IsExpression())
            {
                expressionNode.Visit(m_visitor);
                return m_visitor.GetParseResultAs<AST::Expression>();
            }
            else
            {
                AddError(ParseError("arguments expression expected, incorrect type found"));
                return nullptr;
            }        
        }

        AST::SmartPtrConst<AST::ExpressionList> Parser::ParseExpressionList(const Node& firstExpressionListNode)
        {
            // \todo this doesn't work yet
            ExpressionScoper expressionScoper(*this);
            TraitsVisitor traits;
            firstExpressionListNode.Visit(traits);

            if (traits.IsExpression())
            {
                AZStd::vector<AST::SmartPtrConst<AST::Expression>> expressionList;
                const Node* expressionNode = &firstExpressionListNode;
                
                /*
                // this will have to be cleaned up, perhaps include the source node expecting the expression
                // the number of expression would be capped the node knows how many it is expecting
                while (AST::SmartPtrConst<AST::Expression> expression = ParseExpression(expressionNode))
                {
                    expressionList.emplace_back(expression);
                    expression 
                }
                */

                expressionList.emplace_back(ParseExpression(firstExpressionListNode));

                AST::NodeCtrInfo info(firstExpressionListNode);
                return aznew AST::ExpressionList(info, expressionList);
            }
            else
            {
                AddError(ParseError("arguments expression expected, incorrect type found"));
            }

            return nullptr;
        }

        AST::SmartPtrConst<AST::FunctionCallAsStatement> Parser::ParseFunctionCallAsStatement(const Node& node, const char* functionName)
        {
            const Node& functionCallNode = *GetToken();
            ConsumeToken(); 
            TraitsVisitor traits;
            functionCallNode.Visit(traits);
            
            AST::SmartPtrConst<AST::ExpressionList> args;

            if (traits.ExpectsArguments())
            {
                if (auto expressionNode = GetToken())
                {
                    args = ParseExpressionList(*expressionNode);
                    // check for more errors
                }

                // check for more errors
            }
            else
            {
                // check for more errors
            }

            AST::NodeCtrInfo info(functionCallNode);
            AST::SmartPtrConst<AST::Name> name = aznew AST::Name(AST::NodeCtrInfo(node), functionName);
            AST::SmartPtrConst<AST::Variable> variable = aznew AST::Variable(AST::NodeCtrInfo(node), AZStd::move(name));
            AST::SmartPtrConst<AST::PrefixExpression> prefixExpression = aznew AST::PrefixExpression(AST::NodeCtrInfo(node), AZStd::move(variable));
            AST::SmartPtr<AST::FunctionCallAsStatement> functionCallStatement = aznew AST::FunctionCallAsStatement(info, AZStd::move(prefixExpression), AZStd::move(args)); // function expression, args);
            return functionCallStatement;
        }

        AST::NodePtrConst Parser::ParseFunctionCall(const Node& node, const char* functionName)
         {
            AST::NodePtrConst functionCall;

             if (m_expressionDepth == 0)
             {
                 auto functionCallAsStatement = ParseFunctionCallAsStatement(node, functionName);
                 functionCall = functionCallAsStatement;
                 AddStatement(functionCallAsStatement);
             }
             else
             {
                 AZ_Assert(m_expressionDepth > 0, "expression depth tracking has failed");
                 AZ_Assert(false, "finish this");
                 // auto functionCallAsExpression = ParseFunctionCallAsExpression();
                 // functionCall = functionCallAsExpression;
             }

             return functionCall;
         }

        AST::SmartPtrConst<AST::Numeral> Parser::ParseNumeral(const Node& node)
        {
            ConsumeToken();
            
            if (auto number = azrtti_cast<const Nodes::Math::Number*>(&node))
            {
                AZ_Assert(false, "re-write to new objects!");
                return nullptr; // aznew AST::Numeral(AST::NodeCtrInfo(node), number->GetValue().Get());
            }
            else
            {
                AddError(ParseError("Number expression expected but not found!"));
                return nullptr;
            }
        }

        void Parser::SetRoot(AST::SmartPtr<AST::Block>&& root)
        {
            AZ_Assert(!m_root, "the root node has already been set in the AST");
            m_root = std::move(root);
        }


    } // namespace Grammar

} // namespace ScriptCanvas