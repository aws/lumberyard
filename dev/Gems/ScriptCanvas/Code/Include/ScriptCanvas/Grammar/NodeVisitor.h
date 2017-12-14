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

#define SCRIPT_CANVAS_NODE_VISITOR_FUNCTIONS(PREFIX, SUFFIX)\
    PREFIX Visit(const Node& node) { AZ_WarningOnce("ScriptCanvas", false, "Implement a Visit on all leaf node"); }; /* \todo delete me! */\
    PREFIX Visit(const Nodes::Math::Divide& node) SUFFIX;\
    PREFIX Visit(const Nodes::Math::Multiply& node) SUFFIX;\
    PREFIX Visit(const Nodes::Math::Number& node) SUFFIX;\
    PREFIX Visit(const Nodes::Math::Subtract& node) SUFFIX;\
    PREFIX Visit(const Nodes::Math::Sum& node) SUFFIX;

#define SCRIPT_CANVAS_NODE_VISITOR_FUNCTION_DECLARATIONS\
    SCRIPT_CANVAS_NODE_VISITOR_FUNCTIONS(virtual void, =0)

#define SCRIPT_CANVAS_NODE_VISITOR_FUNCTION_OVERRIDES\
    SCRIPT_CANVAS_NODE_VISITOR_FUNCTIONS(void, override)

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

        SCRIPT_CANVAS_NODE_VISITOR_FUNCTION_DECLARATIONS;
    };
}