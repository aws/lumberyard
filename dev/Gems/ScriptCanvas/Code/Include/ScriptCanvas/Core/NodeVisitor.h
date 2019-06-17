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

#define SCRIPT_CANVAS_NODE_VISITOR_FUNCTIONS(PREFIX, SUFFIX)\
    PREFIX Visit(const Nodes::Core::Start& node) SUFFIX;\
    PREFIX Visit(const Nodes::Debug::Log& node) SUFFIX;\
    /* put your nodes above in alphabetical order, please */

/* the goal should be: no special processing needed
PREFIX Visit(const Nodes::String::Print& node) SUFFIX;\
*/

#define SCRIPT_CANVAS_NODE_VISITOR_FUNCTION_DECLARATIONS\
    SCRIPT_CANVAS_NODE_VISITOR_FUNCTIONS(virtual void, =0)

#define SCRIPT_CANVAS_NODE_VISITOR_FUNCTION_OVERRIDES\
    SCRIPT_CANVAS_NODE_VISITOR_FUNCTIONS(void, override)

namespace ScriptCanvas
{
    class Node;

    namespace Nodes
    {
        namespace Core
        {
            class Start;
        }

        namespace Debug
        {
            class Log;
        }

        namespace String
        {
            class Print;
        }
    }

    class NodeVisitor
    {
        friend class Node;

    public:
        virtual ~NodeVisitor() = default;

        // clients must decide what to do with the base node
        virtual void Visit(const Node& node) = 0;

        SCRIPT_CANVAS_NODE_VISITOR_FUNCTION_DECLARATIONS;
    };
}