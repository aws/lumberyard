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

#include "OperatorLength.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/MathOperatorContract.h>
#include <AzCore/Math/MathUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorLength::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput
                    || sourceType == SourceType::SourceOutput)
                {
                    ContractDescriptor vectorTypeMethodContract;
                    vectorTypeMethodContract.m_createFunc = [this]() -> RestrictedTypeContract* { return aznew RestrictedTypeContract({ Data::Type::Vector2(), Data::Type::Vector3(), Data::Type::Vector4(), Data::Type::Quaternion() });
                    };
                    contractDescs.push_back(AZStd::move(vectorTypeMethodContract));
                }
            }

            void OperatorLength::Operator(Data::eType type, const OperatorOperands& operands, Datum& result)
            {
                switch (type)
                {
                case Data::eType::Vector2:
                {
                    const AZ::Vector2* vector = operands.front()->GetAs<AZ::Vector2>();
                    result = Datum(vector->GetLength());
                }   
                break;
                case Data::eType::Vector3:
                {
                    const AZ::Vector3* vector = operands.front()->GetAs<AZ::Vector3>();
                    result = Datum(vector->GetLength());
                }
                break;
                case Data::eType::Vector4:
                {
                    const AZ::Vector4* vector = operands.front()->GetAs<AZ::Vector4>();
                    result = Datum(vector->GetLength());
                }
                break;
                case Data::eType::Quaternion:
                {
                    const AZ::Quaternion* vector = operands.front()->GetAs<AZ::Quaternion>();
                    result = Datum(vector->GetLength());
                }
                break;
                default:
                    AZ_Assert(false, "Length operator not defined for type: %s", Data::ToAZType(type).ToString<AZStd::string>().c_str());
                    break;
                }
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorLength.generated.cpp>