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

#include "OperatorAdd.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/MathOperatorContract.h>
#include <AzCore/Math/MathUtils.h>
#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            template<typename Type>
            struct OperatorAddImpl
            {
                Type operator()(const Type& a, const Datum& b)
                {
                    const Type* dataB = b.GetAs<Type>();

                    if (dataB)
                    {
                        return a + (*dataB);
                    }
                    else
                    {
                        return a;
                    }
                }
            };

            template<>
            struct OperatorAddImpl<Data::AABBType>
            {
                Data::AABBType operator()(const Data::AABBType& lhs, const Datum& rhs)
                {
                    Data::AABBType retVal = lhs;
                    const AZ::Aabb* dataB = rhs.GetAs<AZ::Aabb>();

                    if (dataB)
                    {
                        retVal.AddAabb((*dataB));
                    }

                    return retVal;
                }
            };

            template <>
            struct OperatorAddImpl<Data::ColorType>
            {
                Data::ColorType operator()(const Data::ColorType& lhs, const Datum& rhs)
                {
                    const AZ::Color* dataB = rhs.GetAs<AZ::Color>();

                    if (dataB)
                    {
                        float a = AZ::GetClamp<float>(lhs.GetA(), 0.f, 1.f) + AZ::GetClamp<float>(dataB->GetA(), 0.f, 1.f);
                        float r = AZ::GetClamp<float>(lhs.GetR(), 0.f, 1.f) + AZ::GetClamp<float>(dataB->GetR(), 0.f, 1.f);
                        float g = AZ::GetClamp<float>(lhs.GetG(), 0.f, 1.f) + AZ::GetClamp<float>(dataB->GetG(), 0.f, 1.f);
                        float b = AZ::GetClamp<float>(lhs.GetB(), 0.f, 1.f) + AZ::GetClamp<float>(dataB->GetB(), 0.f, 1.f);

                        return AZ::Color(r, g, b, a);
                    }
                    else
                    {
                        return lhs;
                    }
                }
            };

            void OperatorAdd::Operator(Data::eType type, const OperatorOperands& operands, Datum& result)
            {
                switch (type)
                {
                case Data::eType::Number:
                    OperatorEvaluator::Evaluate<Data::NumberType>(OperatorAddImpl<Data::NumberType>(), operands, result);
                    break;
                case Data::eType::Color:
                    OperatorEvaluator::Evaluate<Data::ColorType>(OperatorAddImpl<Data::ColorType>(), operands, result);
                    break;
                case Data::eType::Vector2:
                    OperatorEvaluator::Evaluate<Data::Vector2Type>(OperatorAddImpl<Data::Vector2Type>(), operands, result);
                    break;
                case Data::eType::Vector3:
                    OperatorEvaluator::Evaluate<Data::Vector3Type>(OperatorAddImpl<Data::Vector3Type>(), operands, result);
                    break;
                case Data::eType::Vector4:
                    OperatorEvaluator::Evaluate<Data::Vector4Type>(OperatorAddImpl<Data::Vector4Type>(), operands, result);
                    break;
                case Data::eType::String:
                    OperatorEvaluator::Evaluate<Data::StringType>(OperatorAddImpl<Data::StringType>(), operands, result);
                    break;
                case Data::eType::Quaternion:
                    OperatorEvaluator::Evaluate<Data::QuaternionType>(OperatorAddImpl<Data::QuaternionType>(), operands, result);
                    break;
                case Data::eType::AABB:
                    OperatorEvaluator::Evaluate<Data::AABBType>(OperatorAddImpl<Data::AABBType>(), operands, result);
                    break;                    
                default:
                    AZ_Assert(false, "Addition operator not defined for type: %s", Data::ToAZType(type).ToString<AZStd::string>().c_str());
                    break;
                }
            }

            AZStd::unordered_set< Data::Type > OperatorAdd::GetSupportedNativeDataTypes() const
            {
                return{
                    Data::Type::Number(),
                    Data::Type::Vector2(),
                    Data::Type::Vector3(),
                    Data::Type::Vector4(),
                    Data::Type::Color(),
                    Data::Type::Quaternion(),
                    Data::Type::AABB()
                };
            }

            bool OperatorAdd::IsValidArithmeticSlot(const SlotId& slotId) const
            {
                const Datum* datum = GetInput(slotId);

                if (datum)
                {
                    switch (datum->GetType().GetType())
                    {
                    case Data::eType::Number:
                        return !AZ::IsClose((*datum->GetAs<Data::NumberType>()), Data::NumberType(0.0), ScriptCanvas::Data::ToleranceEpsilon());
                    case Data::eType::Quaternion:
                        return !datum->GetAs<Data::QuaternionType>()->IsIdentity();
                    case Data::eType::String:
                        return !datum->GetAs<Data::StringType>()->empty();
                    }
                }

                return true;
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorAdd.generated.cpp>