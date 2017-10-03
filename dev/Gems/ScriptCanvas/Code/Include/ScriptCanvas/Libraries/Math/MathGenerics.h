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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzFramework/Math/MathUtils.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>

namespace ScriptCanvas
{
    /// \note Don't forget to add your Node to the template argument list for MathRegistrar

    AZ_INLINE std::tuple<AZ::Vector3, Data::NumberType> VectorNormalizeWithLength(const AZ::Vector3& source)
    {
        auto sourceCopy(source);
        float lengthFloat;
        sourceCopy.NormalizeSafeWithLength().StoreToFloat(&lengthFloat);
        return std::make_tuple(sourceCopy, lengthFloat);
    }    
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(VectorNormalizeWithLength, "{A9F29CC6-7FBF-400F-92C6-18F28AD256B9}", "Vector", "Normalized", "Length");

    AZ_INLINE AZ::Vector3 VectorAdd(const AZ::Vector3& lhs, const AZ::Vector3& rhs)
    {
        return lhs + rhs;
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(VectorAdd, "{92A4801A-15FB-4529-80BA-B880D8C24989}", "A", "B");

    AZ_INLINE AZ::Vector3 VectorNormalize(const AZ::Vector3& source)
    {
        return source.GetNormalizedSafe();
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(VectorNormalize, "{971E7456-4BDF-4FB3-A418-D6ECAC186FD5}", "Vector");
    
    AZ_INLINE AZ::Vector3 VectorSubtract(const AZ::Vector3& lhs, const AZ::Vector3& rhs)
    {
        return lhs - rhs;
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(VectorSubtract, "{0DE69020-4DB2-4559-9C29-6CD8EAC05F1E}", "A", "B");

    AZ_INLINE Data::RotationType ConvertTransformToRotation(const Data::TransformType& transform)
    {
        return AzFramework::ConvertEulerRadiansToQuaternion(AzFramework::ConvertTransformToEulerRadians(transform));
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ConvertTransformToRotation, "{C878982F-1B6B-4555-8723-7FF3830C8032}", "Transform");

    AZ_INLINE AZ::Vector3 VectorLerp(const AZ::Vector3& from, const AZ::Vector3& to, Data::NumberType time)
    {
        float t = aznumeric_cast<float>(time);
        return from + (to - from) * AZ::VectorFloat::CreateFromFloat(&t);
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(VectorLerp, "{AA063267-DA0F-4407-9356-30B4E89A9FA4}", "From", "To", "Time");

    AZ_INLINE Data::TransformType CreateLookAt(const AZ::Vector3& from, const AZ::Vector3& to, Data::NumberType axisNumber)
    {
        using namespace AzFramework;
        Axis axis = static_cast<Axis>(static_cast<AZ::u32>(axisNumber));
        // default to the default "forward" axis if an erroneous axis is passed in
        axis = axis < Axis::XPositive || axis > Axis::ZNegative ? Axis::YPositive : axis;
        return AzFramework::CreateLookAt(from, to, axis);
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(CreateLookAt, "{D5223A1E-F725-4E67-8E70-2975720F91E8}", "From", "To", "Axis[0:5]");

    AZ_INLINE Data::TransformType CreateLookAtYPosAxis(const AZ::Vector3& from, const AZ::Vector3& to)
    {
        return AzFramework::CreateLookAt(from, to, AzFramework::Axis::YPositive);
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(CreateLookAtYPosAxis, "{8BD2AB7A-AF2E-4748-9530-71055E5EA986}", "From", "To");
        
    using MathRegistrar = RegistrarGeneric
        < ConvertTransformToRotationNode
        , CreateLookAtNode
        , CreateLookAtYPosAxisNode
        , VectorAddNode
        , VectorLerpNode
        , VectorNormalizeNode
        , VectorSubtractNode
        , VectorNormalizeWithLengthNode
        >;
} // namespace ScriptCanvas

