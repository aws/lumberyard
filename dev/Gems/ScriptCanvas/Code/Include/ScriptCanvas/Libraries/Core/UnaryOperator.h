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
        class UnaryOperator
            : public Node
        {
        public:
            AZ_COMPONENT(UnaryOperator, "{B0BF8615-D718-4115-B3D8-CAB554BC6863}", Node);

            static void Reflect(AZ::ReflectContext* reflection);

            static const char* k_valueName;
            static const char* k_resultName;

            static const char* k_evaluateName;
            static const char* k_onTrue;
            static const char* k_onFalse;

        protected:

            // Indices into m_inputData
            static const int k_datumIndex = 0;

            // Indices into m_slotContainer.m_slots
            int m_outputSlotIndex = -1;

            void ConfigureSlots() override;

            // must be overridden with the binary operations
            virtual Datum Evaluate(const Datum& value);

            // Triggered by the execution signal
            void OnInputSignal(const SlotId& slot) override;
        };

        class UnaryExpression : public UnaryOperator
        {
        public:
            AZ_COMPONENT(UnaryExpression, "{70FF2162-3D01-41F1-B009-7DC071A38471}", UnaryOperator);

            static void Reflect(AZ::ReflectContext* reflection);

        protected:

            void ConfigureSlots() override;

            void OnInputSignal(const SlotId& slot) override;

            virtual void InitializeUnaryExpression();
        };
    }
}

#if defined(EXPRESSION_TEMPLATES_ENABLED)

#include "precompiled.h"

#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        template<typename Derived, typename UnaryFunctor>
        class UnaryOperator
            : public Node
        {
        public:
            AZ_STATIC_ASSERT(AZStd::function_traits<UnaryFunctor>::num_args == 1, "Unary member function must accept two arguments");
            using ResultType = std::remove_cv_t<std::remove_reference_t<typename AZStd::function_traits<UnaryFunctor>::result_type>>;
            using FirstArgType = std::remove_cv_t<std::remove_reference_t<AZStd::function_traits_get_arg_t<UnaryFunctor, 0>>>;


            AZ_RTTI((UnaryOperator, "{51ABCD80-C005-462B-8A6A-844405EAE4EA}", Derived), Node, AZ::Component);
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(UnaryOperator);
            AZ_COMPONENT_BASE(UnaryOperator)

            static void Reflect(AZ::ReflectContext* reflection);

            UnaryOperator() = default;
            ~UnaryOperator() override = default;

            void OnEntry() override;
            void OnInputSignal(const SlotId&) override;

            static const char* GetFirstArgName() { return "A"; }
            static const char* GetResultName() { return "This"; }

        protected:
            static const char* GetOperatorName() { return "UnaryOperator"; }
            static const char* GetOperatorDesc() { return "Performs Unary Operator on the provided argument"; }
            static const char* GetIconPath() { return ""; }
            static AZStd::vector<ContractDescriptor> GetFirstArgContractDesc() { return AZStd::vector<ContractDescriptor>{}; }
            static AZStd::vector<ContractDescriptor> GetResultContractDesc() { return AZStd::vector<ContractDescriptor>{}; }

            template<typename ValueType>
            bool SetValueOnFirstEndpoint(ValueType& property, const SlotId& slotId);

        private:
            FirstArgType m_arg1{};
            ResultType m_result{};

        };

        template<typename Derived, typename UnaryFunctor>
        void UnaryOperator<Derived, UnaryFunctor>::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                // Reflect UnaryOperator as a derived class from the base Node class
                serializeContext->Class<UnaryOperator, Node>()
                    ->Version(0)
                    ;

                // Reflect Derived class to the serialization context
                serializeContext->Class<Derived, UnaryOperator>()
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

        template<typename Derived, typename UnaryFunctor>
        void UnaryOperator<Derived, UnaryFunctor>::Initialize(const AZ::Uuid& classId)
        {
            AddSlot({ Derived::GetFirstArgName(), SlotType::DataIn, Derived::GetFirstArgContractDesc() });
            AddSlot({ Derived::GetResultName(), SlotType::DataOut, Derived::GetResultContractDesc() });
        }

        template<typename Derived, typename UnaryFunctor>
        void UnaryOperator<Derived, UnaryFunctor>::OnEntry()
        {
            m_status = ExecutionStatus::Immediate;
            m_executionMode = ExecutionStatus::Immediate;
        }

        template<typename Derived, typename UnaryFunctor>
        void UnaryOperator<Derived, UnaryFunctor>::OnInputSignal(const SlotId&)
        {
            if (m_status != ExecutionStatus::NotStarted)
            {
                m_executionMode = ExecutionStatus::Done;
            }
        }
        
    } // namespace Nodes
} // namespace ScriptCanvas

#endif // defined(EXPRESSION_TEMPLATES_ENABLED)
