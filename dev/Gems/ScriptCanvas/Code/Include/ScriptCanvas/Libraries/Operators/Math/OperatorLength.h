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

#include "OperatorArithmetic.h"
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorLength.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorLength : public OperatorArithmeticUnary
            {
            public:

                ScriptCanvas_Node(OperatorLength,
                    ScriptCanvas_Node::BaseClass("OperatorArithmetic")
                    ScriptCanvas_Node::Name("Length")
                    ScriptCanvas_Node::Uuid("{AEE15BEA-CD51-4C1A-B06D-C09FB9EAA005}")
                    ScriptCanvas_Node::Description("Returns the length of the specified source")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Math")
                );

                struct LengthOperatorConfiguration
                    : public UnaryArithmeticOperatorConfiguration
                {
                    LengthOperatorConfiguration()
                    {
                        OperatorBase::SourceSlotConfiguration inputSourceConfig;

                        inputSourceConfig.m_dynamicDataType = DynamicDataType::Value;
                        inputSourceConfig.m_name = "Source";
                        inputSourceConfig.m_tooltip = "The object to take the lenght of";
                        inputSourceConfig.m_sourceType = OperatorBase::SourceType::SourceInput;

                        m_sourceSlotConfigurations.emplace_back(inputSourceConfig);
                    }
                };

                OperatorLength()
                    : OperatorArithmeticUnary(LengthOperatorConfiguration())
                {
                }

                virtual AZStd::string_view OperatorFunction() const { return "Length"; }

                void Operator(Data::eType type, const OperatorOperands& operands, Datum& result) override;

                void OnSourceTypeChanged() override
                {
                    Data::Type type = Data::Type::Number();
                    m_outputSlots.insert(AddOutputTypeSlot(Data::GetName(type), "Result", type, Node::OutputStorage::Required, false));
                }

                void ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs) override;

            };
        }
    }
}