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

#include "NodeVisitor.h"

#include <ScriptCanvas/AST/AST.h>
#include <ScriptCanvas/AST/Nodes.h>

namespace ScriptCanvas
{
    namespace Grammar
    {
        class Parser;

        class ParserVisitor
            : public NodeVisitor
        {
        public:
            ParserVisitor(Parser& parser);
            
            AZ_INLINE AST::NodePtrConst GetParseResult() const { return m_result; }
            
            template<typename t_NodeType>
            AZ_INLINE AST::SmartPtrConst<t_NodeType> GetParseResultAs() const
            {
                return azrtti_cast<const t_NodeType*>(m_result);
            }
            
            SCRIPT_CANVAS_NODE_VISITOR_FUNCTION_OVERRIDES;

        private:
            AST::NodePtrConst m_result;
            Parser& m_parser;
        };
    }
}