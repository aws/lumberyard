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
#include "BinaryOperator.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        const char* BinaryOperator::k_evaluateName = "In";
        const char* BinaryOperator::k_outName = "Out";
        const char* BinaryOperator::k_onTrue = "True";
        const char* BinaryOperator::k_onFalse = "False";

        const char* BinaryOperator::k_lhsName = "Value A";
        const char* BinaryOperator::k_rhsName = "Value B";
        const char* BinaryOperator::k_resultName = "Result";
        
        void ArithmeticExpression::OnInit()
        {
            AddOutputTypeSlot(k_resultName, "", Data::Type::Number(), OutputStorage::Optional);

            BinaryOperator::OnInit();

            AddSlot(k_outName, "Signaled after the arithmetic operation is done.", ScriptCanvas::SlotType::ExecutionOut);

            AddInputDatumSlot(k_lhsName, "", AZStd::move(Data::Type::Number()), Datum::eOriginality::Original);
            AddInputDatumSlot(k_rhsName, "", AZStd::move(Data::Type::Number()), Datum::eOriginality::Original);
        }

        void ArithmeticExpression::OnInputSignal(const SlotId& slotId)
        {
            if (slotId == GetSlotId(k_evaluateName))
            {
                const Datum output = Evaluate(*GetDatumByIndex(k_datumIndexLHS), *GetDatumByIndex(k_datumIndexRHS));
                if (auto slot = GetSlot(GetOutputSlotId()))
                {
                    PushOutput(output, *slot);
                }

                SignalOutput(GetSlotId(k_outName));
            }
        }

        void ArithmeticExpression::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<ArithmeticExpression, BinaryOperator>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ArithmeticExpression>("ArithmeticExpression", "ArithmeticExpression")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }
        }

        Datum BinaryOperator::Evaluate(const Datum& lhs, const Datum& rhs)
        {
            AZ_Assert(false, "Evaluate must be overridden");
            return Datum();
        };

        void BinaryOperator::OnInputSignal(const SlotId& slot)
        {
            AZ_Assert(false, "OnInputSignal must be overridden");
        }

        void BinaryOperator::OnInit()
        {
            AddSlot(k_evaluateName, "Signal to perform the evaluation when desired.", ScriptCanvas::SlotType::ExecutionIn);
        }

        SlotId BinaryOperator::GetOutputSlotId() const
        {
            return GetSlotId(k_resultName);
        }

        void BinaryOperator::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<BinaryOperator, Node>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<BinaryOperator>("BinaryOperator", "BinaryOperator")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }
        }

        void BooleanExpression::InitializeBooleanExpression()
        {
            AZ_Assert(false, "InitializeBooleanExpression must be overridden");
        }

        void BooleanExpression::OnInputSignal(const SlotId& slotId)
        {
            if (slotId == GetSlotId(k_evaluateName))
            {
                const Datum output = Evaluate(*GetDatumByIndex(k_datumIndexLHS), *GetDatumByIndex(k_datumIndexRHS));
                if (auto slot = GetSlot(GetOutputSlotId()))
                {
                    PushOutput(output, *slot);
                }

                const bool* result = output.GetAs<bool>();
                if (result && *result)
                {
                    SignalOutput(GetSlotId(k_onTrue));
                }
                else
                {
                    SignalOutput(GetSlotId(k_onFalse));
                }
            }
        }

        void BooleanExpression::OnInit()
        {
            AddOutputTypeSlot(k_resultName, "", Data::Type::Boolean(), OutputStorage::Optional);

            BinaryOperator::OnInit();

            AddSlot(k_onTrue, "Signaled if the result of the operation is true.", ScriptCanvas::SlotType::ExecutionOut);
            AddSlot(k_onFalse, "Signaled if the result of the operation is false.", ScriptCanvas::SlotType::ExecutionOut);

            InitializeBooleanExpression();

        }

        void BooleanExpression::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<BooleanExpression, BinaryOperator>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<BooleanExpression>("BooleanExpression", "BooleanExpression")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }
        }
        
        void ComparisonExpression::InitializeBooleanExpression()
        {
            EqualityExpression::InitializeBooleanExpression();
            AddOutputTypeSlot(k_resultName, "", Data::Type::Boolean(), OutputStorage::Optional);
        }

        static bool ComparisonExpressionVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
        {
            if (rootElement.GetVersion() == 0)
            {
                int nodeElementIndex = rootElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));
                if (nodeElementIndex == -1)
                {
                    AZ_Error("Script Canvas", false, "Unable to find base class node element for ComparisonExpression class %s", false, rootElement.GetNameString());
                    return false;
                }

                // There was change to the ComparisonExpression Base Class type from BooleanExpression to ComparisonExpression.
                // Because the version was not incremented when this change took place version 0 instances of ComparisonExpression class will fail to load when the base class
                // is BinaryExpression
                // Explicitly making a copy of the baseNodeElement notice there is no "&" after AZ::SerializeContext::DataElementNode
                AZ::SerializeContext::DataElementNode baseNodeElement = rootElement.GetSubElement(nodeElementIndex);
                if (baseNodeElement.GetId() == azrtti_typeid<BooleanExpression>())
                {
                    rootElement.RemoveElement(nodeElementIndex);
                    int equalityExpressionElementIndex = rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<EqualityExpression>());
                    if (equalityExpressionElementIndex == -1)
                    {
                        AZ_Error("Script Canvas", false, "Unable to convert BooleanExpression data element to ComparisonExpression data element");
                        return false;
                    }

                    auto& equalityExpressionElement = rootElement.GetSubElement(equalityExpressionElementIndex);
                    if (equalityExpressionElement.AddElement(baseNodeElement) == -1)
                    {
                        AZ_Error("Script Canvas", false, "Unable to add boolean expression data element node as a base class to the equality expression data element node");
                        return false;
                    }
                }
            }
            return true;
        }

        void ComparisonExpression::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<ComparisonExpression, EqualityExpression>()
                    ->Version(1, &ComparisonExpressionVersionConverter)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ComparisonExpression>("ComparisonExpression", "ComparisonExpression")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }
        }
        
        void EqualityExpression::InitializeBooleanExpression()
        {
            AddInputDatumOverloadedSlot(k_lhsName);
            AddInputDatumOverloadedSlot(k_rhsName);
        }

        void EqualityExpression::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<EqualityExpression, BooleanExpression>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EqualityExpression>("EqualityExpression", "EqualityExpression")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }
        }
    }
}
