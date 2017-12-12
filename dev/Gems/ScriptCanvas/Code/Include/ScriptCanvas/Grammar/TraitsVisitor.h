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

#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
    namespace Grammar
    {
        class TraitsVisitor 
            : public NodeVisitor
        {
        public:
            TraitsVisitor();
            
            SCRIPT_CANVAS_NODE_VISITOR_FUNCTION_OVERRIDES;

            AZ_INLINE bool IsPureData() const { return m_isPureData; }
            
            AZ_INLINE bool IsFunctionCall() const { return m_isFunctionCall; }

            AZ_INLINE bool IsExpression() const { return m_isExpression; }

            AZ_INLINE bool ExpectsArguments() const { return m_expectsArguments; }

            AZ_INLINE bool IsInitialStatement() const { return m_isInitialStatement; }

            // function call nodes that look like expressions must be statements if:
            // * they have output consumed in more than one input node
            // * they have no output
            AZ_INLINE bool IsStatement() const { return m_isInitialStatement; }

        protected:            
            static bool IsPrecededByStatementNode(const Node& node);
            static bool IsStartNode(const Node& node);
            
            void ResetTraits();

        private:
            bool m_expectsArguments;
            bool m_isExpression;
            bool m_isFunctionCall;
            bool m_isInitialStatement;
            bool m_isPureData;
            bool m_isStatement;
        };
    }
}