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

#include "precompiled.h"

#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/PureData.h>
#include <ScriptCanvas/Core/Datum.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        class BinaryOperator
            : public Node
        {
        public:
            AZ_COMPONENT(BinaryOperator, "{5BD0E8C7-9B0A-42F5-9EB0-199E6EC8FA99}", Node);

            static void Reflect(AZ::ReflectContext* reflection);
            
            static const char* k_lhsName;
            static const char* k_rhsName;
            static const char* k_resultName;

            static const char* k_evaluateName;
            static const char* k_outName;
            static const char* k_onTrue;
            static const char* k_onFalse;

        protected:

            // Indices into m_inputData
            static const int k_datumIndexLHS = 0;
            static const int k_datumIndexRHS = 1;

            // Indices into m_slotContainer.m_slots
            int m_outputSlotIndex = -1;

            void OnInit() override;

            // must be overridden with the binary operations
            virtual Datum Evaluate(const Datum& lhs, const Datum& rhs);

            // Triggered by the execution signal
            void OnInputSignal(const SlotId& slot) override;
        };
                
        class ArithmeticExpression
            : public BinaryOperator
        {
        public:
            AZ_COMPONENT(ArithmeticExpression, "{B13F8DE1-E017-484D-9910-BABFB355D72E}", BinaryOperator);

            static void Reflect(AZ::ReflectContext* reflection);

        protected:
            // adds Number inputs, adds Number output type
            void OnInit() override;

            void OnInputSignal(const SlotId& slot) override;

        };
        
        class BooleanExpression
            : public BinaryOperator
        {
        public:
            AZ_COMPONENT(BooleanExpression, "{36C69825-CFF8-4F70-8F3B-1A9227E8BEEA}", BinaryOperator);
            
            static void Reflect(AZ::ReflectContext* reflection);

        protected:
            // initialize boolean expression, adds boolean output type calls 
            void OnInit() override;
            virtual void InitializeBooleanExpression();
            void OnInputSignal(const SlotId& slot) override;
        };

        // accepts any type, checks for type equality, and then value equality or pointer equality
        class EqualityExpression
            : public BooleanExpression
        {
        public:
            AZ_COMPONENT(EqualityExpression, "{78D20EB6-BA07-4071-B646-7C2D68A0A4A6}", BooleanExpression);
            
            static void Reflect(AZ::ReflectContext* reflection);

        protected:
            // adds any required input types
            void InitializeBooleanExpression() override;
        };

        // accepts numbers only
        class ComparisonExpression
            : public EqualityExpression
        {
        public:
            AZ_COMPONENT(ComparisonExpression, "{82C50EAD-D3DD-45D2-BFCE-981D95771DC8}", EqualityExpression);

            static void Reflect(AZ::ReflectContext* reflection);
        
        protected:
            // adds number types
            void InitializeBooleanExpression() override;
        };

#if defined(EXPRESSION_TEMPLATES_ENABLED)
        template<typename Derived, typename BinaryFunctor>
        class BinaryOperatorGeneric
            : public Node
        {
        public:
            AZ_STATIC_ASSERT(AZStd::function_traits<BinaryFunctor>::num_args == 2, "Binary member function must accept two arguments");
            using ResultType = std::remove_cv_t<std::remove_reference_t<typename AZStd::function_traits<BinaryFunctor>::result_type>>;
            using FirstArgType = std::remove_cv_t<std::remove_reference_t<AZStd::function_traits_get_arg_t<BinaryFunctor, 0>>>;
            using SecondArgType = std::remove_cv_t<std::remove_reference_t<AZStd::function_traits_get_arg_t<BinaryFunctor, 1>>>;
            
            AZ_RTTI((BinaryOperatorGeneric, "{45E0919C-C831-4B49-BE1F-FF13BC100F49}", Derived), Node, AZ::Component);
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(BinaryOperatorGeneric);
            AZ_COMPONENT_BASE(BinaryOperatorGeneric)

            static void Reflect(AZ::ReflectContext* reflection);

            BinaryOperatorGeneric() = default;
            ~BinaryOperatorGeneric() override = default;

            static const char* GetFirstArgName() { return "Arg1"; }
            static const char* GetSecondArgName() { return "Arg2"; }
            static const char* GetResultName() { return "Result"; }

        protected:
            static const char* GetOperatorName() { return "BinaryOperatorGeneric"; }
            static const char* GetOperatorDesc() { return "Performs Binary Operator on two provided arguments"; }
            static const char* GetIconPath() { return ""; }
            static AZStd::vector<ContractDescriptor> GetFirstArgContractDesc() { return AZStd::vector<ContractDescriptor>{}; }
            static AZStd::vector<ContractDescriptor> GetSecondArgContractDesc() { return AZStd::vector<ContractDescriptor>{}; }
            static AZStd::vector<ContractDescriptor> GetResultContractDesc() { return AZStd::vector<ContractDescriptor>{}; }

            template<typename ValueType>
            bool SetValueOnFirstEndpoint(ValueType& property, const SlotId& slotId);

        private:
            FirstArgType m_arg1{};
            SecondArgType m_arg2{};
            ResultType m_result{};
        };

        template<typename Derived, typename BinaryFunctor>
        void BinaryOperatorGeneric<Derived, BinaryFunctor>::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                // Reflect BinaryOperatorGeneric as a derived class from the base Node class
                serializeContext->Class<BinaryOperatorGeneric, Node>()
                    ->Version(0)
                    ;

                // Reflect Derived class to the serialization context
                serializeContext->Class<Derived, BinaryOperatorGeneric>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Derived>(Derived::GetOperatorName(), Derived::GetOperatorDesc())
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, Derived::GetIconPath())
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
            }
        }

        template<typename Derived, typename BinaryFunctor>
        void BinaryOperatorGeneric<Derived, BinaryFunctor>::Initialize(const AZ::Uuid& classId)
        {
            AddSlot({ Derived::GetFirstArgName(), SlotType::DataIn, Derived::GetFirstArgContractDesc() });
            AddSlot({ Derived::GetSecondArgName(), SlotType::DataIn, Derived::GetSecondArgContractDesc() });
            AddSlot({ Derived::GetResultName(), SlotType::DataOut, Derived::GetResultContractDesc() });
        }
#endif // defined(EXPRESSION_TEMPLATES_ENABLED)

    } // namespace Nodes
} // namespace ScriptCanvas
