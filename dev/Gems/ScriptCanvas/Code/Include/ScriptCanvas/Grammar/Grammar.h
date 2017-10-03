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

namespace ScriptCanvas
{
    namespace Grammar
    {
        enum class eRule : AZ::u32
        {
            AddOperation,
            AssignmentStatement,
            BinaryOperation,
            Block,
            BoolExpression,
            Break,
            Declaration,
            DeclarationList,
            DivideOperation,
            EntityIDExpression,
            Expression,
            ExpressionList,
            FunctionBody,
            FunctionCall,
            FunctionDefinition,
            FunctionSignature,
            IfStatement,
            InitializationStatement,
            ListExpression,
            MultiplyOperation,
            Name,
            Next,
            Numeral,
            ObjectExpression,
            PrefixExpression,
            RepeatStatement,
            ReturnStatement,
            Scope,
            Statement,
            SubtractOperation,
            TransformExpression,
            UnaryOperation,
            Variable,
            VariableList,
            VectorExpression,
            WhileStatement,
            
            Count,
        };
    }
}