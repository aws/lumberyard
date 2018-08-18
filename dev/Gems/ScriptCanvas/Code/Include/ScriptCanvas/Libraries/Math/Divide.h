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

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Libraries/Core/BinaryOperator.h>
#include <ScriptCanvas/Libraries/Math/ArithmeticFunctions.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            class Divide
                : public ArithmeticExpression
            {
            public:
                AZ_COMPONENT(Divide, "{7379D5B4-787B-4C46-9394-288F16E5BF3A}", ArithmeticExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Divide, ArithmeticExpression>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Divide>("Divide", "Divide")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Placeholder.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }

                void Visit(NodeVisitor& visitor) const override
                {
                    visitor.Visit(*this);
                }

            protected:
                Datum Evaluate(const Datum& lhs, const Datum& rhs) override
                {
                    const Data::NumberType lhsValue = *lhs.GetAs<Data::NumberType>();
                    const Data::NumberType rhsValue = *rhs.GetAs<Data::NumberType>();
                    return Datum(!AZ::IsClose(rhsValue, 0.0, 0.0001) ? lhsValue / rhsValue : lhsValue);
                }
            };

#if defined(EXPRESSION_TEMPLATES_ENABLED)
            class Divide
                : public BinaryOperatorGeneric<Divide, ArithmeticOperator<OperatorType::Div>>
            {
            public:

                using BaseType = BinaryOperatorGeneric<Divide, ArithmeticOperator<OperatorType::Div>>;
                AZ_COMPONENT(Divide, "{A8573017-E81E-47A6-BE1A-F019ED7F55E4}", BaseType);

                static const char* GetOperatorName() { return "Divide"; }
                static const char* GetOperatorDesc() { return "Perform division between two numbers"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Divide.png"; }

                void Visit(NodeVisitor& visitor) const override { visitor.Visit(*this); }

            };
#endif // #if defined(EXPRESSION_TEMPLATES_ENABLED)
        }
    }
}

