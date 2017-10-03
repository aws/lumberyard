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

        const char* BinaryOperator::k_lhsName = "Arg1";
        const char* BinaryOperator::k_rhsName = "Arg2";
        const char* BinaryOperator::k_resultName = "Result";
        
        void ArithmeticExpression::OnInit()
        {
            SlotId slotId = AddOutputTypeSlot(k_resultName, "", Data::Type::Number(), OutputStorage::Optional);
            const Slot* slot{};
            GetValidSlotIndex(slotId, slot, m_outputSlotIndex);

            BinaryOperator::OnInit();

            AddSlot(k_outName, "Signaled after the arithmetic operation is done.", ScriptCanvas::SlotType::ExecutionOut);

            AddInputDatumSlot(k_lhsName, "", AZStd::move(Data::Type::Number()), Datum::eOriginality::Original);
            AddInputDatumSlot(k_rhsName, "", AZStd::move(Data::Type::Number()), Datum::eOriginality::Original);
        }

        void ArithmeticExpression::OnInputSignal(const SlotId& slot)
        {
            AZ_Assert(m_outputSlotIndex != -1, "m_outputSlotIndex was not configured to the index of the result slot");
            if (slot == GetSlotId(k_evaluateName))
            {
                const Datum output = Evaluate(m_inputData[k_datumIndexLHS], m_inputData[k_datumIndexRHS]);
                PushOutput(output, m_slotContainer.m_slots[m_outputSlotIndex]);

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
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
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
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
            }
        }

        void BooleanExpression::InitializeBooleanExpression()
        {
            AZ_Assert(false, "InitializeBooleanExpression must be overridden");
        }

        void BooleanExpression::OnInputSignal(const SlotId& slot)
        {
            AZ_Assert(m_outputSlotIndex != -1, "m_outputSlotIndex was not configured to the index of the result slot");
            if (slot == GetSlotId(k_evaluateName))
            {
                const Datum output = Evaluate(m_inputData[k_datumIndexLHS], m_inputData[k_datumIndexRHS]);
                PushOutput(output, m_slotContainer.m_slots[m_outputSlotIndex]);

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
            SlotId slotId = AddOutputTypeSlot(k_resultName, "", Data::Type::Boolean(), OutputStorage::Optional);
            const Slot* slot{};
            GetValidSlotIndex(slotId, slot, m_outputSlotIndex);

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
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
            }
        }
        
        void ComparisonExpression::InitializeBooleanExpression()
        {
            EqualityExpression::InitializeBooleanExpression();
            AddOutputTypeSlot(k_resultName, "", Data::Type::Boolean(), OutputStorage::Optional);
        }

        void ComparisonExpression::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<ComparisonExpression, EqualityExpression>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ComparisonExpression>("ComparisonExpression", "ComparisonExpression")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
            }
        }
        
        void EqualityExpression::InitializeBooleanExpression()
        {
            AddInputDatumUntypedSlot(k_lhsName);
            AddInputDatumUntypedSlot(k_rhsName);
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
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
            }
        }
    }
}
