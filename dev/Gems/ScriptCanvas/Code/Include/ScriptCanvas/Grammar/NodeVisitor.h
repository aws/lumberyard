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

// sooner rather than later,
// this should get turned into Nodes::FunctionCall, Node::Expression, Nodes::Handler etc
// one thing to remember is that a function call that sends output into two different nodes' input
// is a function call statement (assigning output to variables, NOT a function call expression)

#define SCRIPT_CANVAS_NODE_VISITOR_FUNCTION_OVERRIDES\
    void Visit(const Node&) override {}; /* \todo delete me! */\
    void Visit(const Nodes::Math::Divide& node) override;\
    void Visit(const Nodes::Math::Multiply& node) override;\
    void Visit(const Nodes::Math::Number& node) override;\
    void Visit(const Nodes::Math::Subtract& node) override;\
    void Visit(const Nodes::Math::Sum& node) override;

namespace ScriptCanvas
{
    class Node;

    namespace Nodes
    {
        namespace Math
        {
            class Divide;
            class Multiply;
            class Number;
            class Subtract;
            class Sum;
        }
    }

    class NodeVisitor
    {
        friend class Node;

    public:
        virtual ~NodeVisitor() = default;

        virtual void Visit(const Node&) = 0; // \todo delete me!

        virtual void Visit(const Nodes::Math::Divide&) = 0;
        virtual void Visit(const Nodes::Math::Multiply&) = 0;
        virtual void Visit(const Nodes::Math::Number&) = 0;
        virtual void Visit(const Nodes::Math::Subtract&) = 0;
        virtual void Visit(const Nodes::Math::Sum&) = 0;
    };
}