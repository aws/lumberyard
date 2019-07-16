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

#include <ScriptCanvas/Libraries/Operators/Operator.h>
#include <ScriptCanvas/Data/Data.h>
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorArithmetic.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            inline namespace OperatorEvaluator
            {
                template <typename ResultType, typename OperatorFunctor>
                static void Evaluate(OperatorFunctor&& operatorFunctor, const OperatorBase::OperatorOperands& operands, Datum& result)
                {
                    OperatorBase::OperatorOperands::const_iterator operandIter = operands.begin();

                    ResultType resultType;

                    while (operandIter != operands.end())
                    {
                        const ResultType* type = (*operandIter)->GetAs<ResultType>();

                        if (type)
                        {
                            resultType = (*type);
                            break;
                        }
                    }

                    for (++operandIter; operandIter != operands.end(); ++operandIter)
                    {
                        resultType = AZStd::invoke(operatorFunctor, resultType, (*(*operandIter)));
                    }

                    result = Datum(resultType);
                }
            }

            class OperatorArithmetic : public OperatorBase
            {
            public:

                ScriptCanvas_Node(OperatorArithmetic,
                    ScriptCanvas_Node::Name("OperatorArithmetic")
                    ScriptCanvas_Node::Uuid("{FE0589B0-F835-4CD5-BBD3-86510CBB985B}")
                    ScriptCanvas_Node::Description("")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Operators")
                );

                struct ArithmeticOperatorConfiguration
                    : public OperatorBase::OperatorConfiguration
                {
                    bool m_unary = false;
                };

                OperatorArithmetic()
                    : OperatorBase(ArithmeticOperatorConfiguration())
                {
                }

                OperatorArithmetic(const ArithmeticOperatorConfiguration& operatorConfiguration)
                    : OperatorBase(operatorConfiguration)
                    , m_unary(operatorConfiguration.m_unary)
                {}

                virtual AZStd::string_view OperatorFunction() const { return ""; }

                virtual AZStd::unordered_set< Data::Type > GetSupportedNativeDataTypes() const
                {
                    return {
                        Data::Type::Number(),
                        Data::Type::Vector2(),
                        Data::Type::Vector3(),
                        Data::Type::Vector4(),
                        Data::Type::Color(),
                        Data::Type::Quaternion(),
                        Data::Type::Transform(),
                        Data::Type::Matrix3x3(),
                        Data::Type::Matrix4x4()
                    };
                }

                void ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs) override;
                void OnSourceTypeChanged() override;
                void OnSourceConnected(const SlotId& sourceSlotId) override;
                void OnSourceDisconnected(const SlotId& sourceSlotId) override;
                void OnInputSignal(const SlotId& slotId) override;
                virtual void InvokeOperator();

                void OnDataInputSlotConnected(const SlotId& slotId, const Endpoint& endpoint) override;
                void OnDataInputSlotDisconnected(const SlotId& slotId, const Endpoint& endpoint) override;

                void OnInputChanged(const Datum& input, const SlotId& slotID) override;

                void Evaluate(const OperatorOperands&, Datum&) override;

                virtual void Operator(Data::eType type, const OperatorOperands& operands, Datum& result);

                ScriptCanvas_SerializeProperty(bool, m_unary);

            protected:

                void SetSourceNames(const AZStd::string& inputName, const AZStd::string& outputName);
                void HandleDynamicSlots();

                virtual bool IsValidArithmeticSlot(const SlotId& slotId) const;

                // Contains the list of slots that have, or have the potential, to have values which will impact
                // the specific arithmetic operation.
                //
                // Used at run time to try to avoid invoking extra operator calls for no-op operations
                AZStd::vector< SlotId > m_applicableInputSlots;

                SlotId m_outputSlot;
            };

            class OperatorArithmeticUnary : public OperatorArithmetic
            {
            public:

                ScriptCanvas_Node(OperatorArithmeticUnary,
                    ScriptCanvas_Node::Name("OperatorArithmeticUnary")
                    ScriptCanvas_Node::Uuid("{4B68DF49-35DE-48CF-BCE3-F892CCF2639D}")
                    ScriptCanvas_Node::Description("")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Operators/Math")
                );

                struct UnaryArithmeticOperatorConfiguration
                    : public ArithmeticOperatorConfiguration
                {
                    UnaryArithmeticOperatorConfiguration()
                    {
                        m_unary = true;
                    }
                };

                OperatorArithmeticUnary()
                    : OperatorArithmeticUnary(UnaryArithmeticOperatorConfiguration())
                {
                }

                OperatorArithmeticUnary(const UnaryArithmeticOperatorConfiguration& unaryArithmeticOperator)
                    : OperatorArithmetic(unaryArithmeticOperator)
                {}

                void Evaluate(const OperatorOperands& operands, Datum& result) override
                {
                    if (operands.empty())
                    {
                        return;
                    }

                    AZ_Assert(operands.size() == 1, "Unary arithmetic operators may not have more than one operand");

                    const Datum* operand = operands.front();                                
                    auto type = operand->GetType();
                    if (type.GetType() != Data::eType::BehaviorContextObject)
                    {
                        Operator(type.GetType(), operands, result);
                    }
                }
            };

            struct GeneralMathOperatorConfiguration
                : public OperatorArithmetic::ArithmeticOperatorConfiguration
            {
                GeneralMathOperatorConfiguration()
                {
                    OperatorBase::SourceSlotConfiguration inputSourceConfig;

                    inputSourceConfig.m_dynamicDataType = DynamicDataType::Value;
                    inputSourceConfig.m_name = "Value A";
                    inputSourceConfig.m_tooltip = "A value to be operated on";
                    inputSourceConfig.m_sourceType = OperatorBase::SourceType::SourceInput;

                    m_sourceSlotConfigurations.reserve(3);
                    m_sourceSlotConfigurations.emplace_back(inputSourceConfig);

                    inputSourceConfig.m_name = "Value B";
                    m_sourceSlotConfigurations.emplace_back(inputSourceConfig);

                    OperatorBase::SourceSlotConfiguration outputSourceConfig;

                    outputSourceConfig.m_dynamicDataType = DynamicDataType::Value;
                    outputSourceConfig.m_name = "Result";
                    outputSourceConfig.m_tooltip = "The result of the operation";
                    outputSourceConfig.m_sourceType = OperatorBase::SourceType::SourceOutput;

                    m_sourceSlotConfigurations.emplace_back(outputSourceConfig);
                }
            };
        }
    }
}