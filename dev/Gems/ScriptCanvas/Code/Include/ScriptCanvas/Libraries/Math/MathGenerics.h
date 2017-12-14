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

    AZ_INLINE Data::RotationType ConvertTransformToRotation(const Data::TransformType& transform)
    {
        return AzFramework::ConvertEulerRadiansToQuaternion(AzFramework::ConvertTransformToEulerRadians(transform));
    }
    
    AZ_INLINE Data::TransformType CreateLookAt(const AZ::Vector3& from, const AZ::Vector3& to, Data::NumberType axisNumber)
    {
        using namespace AzFramework;
        Axis axis = static_cast<Axis>(static_cast<AZ::u32>(axisNumber));
        // default to the default "forward" axis if an erroneous axis is passed in
        axis = axis < Axis::XPositive || axis > Axis::ZNegative ? Axis::YPositive : axis;
        return AzFramework::CreateLookAt(from, to, axis);
    }
    
    AZ_INLINE Data::TransformType CreateLookAtYPosAxis(const AZ::Vector3& from, const AZ::Vector3& to)
    {
        return AzFramework::CreateLookAt(from, to, AzFramework::Axis::YPositive);
    }
    
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ConvertTransformToRotation, "Math/Rotation", "{C878982F-1B6B-4555-8723-7FF3830C8032}", "", "Transform");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(CreateLookAt, "Math/Transform", "{D5223A1E-F725-4E67-8E70-2975720F91E8}", "", "From", "To", "Axis[0:5]");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(CreateLookAtYPosAxis, "Math/Transform", "{8BD2AB7A-AF2E-4748-9530-71055E5EA986}", "", "From", "To");
        
    using MathRegistrar = RegistrarGeneric
        < ConvertTransformToRotationNode

#if ENABLE_EXTENDED_MATH_SUPPORT
        , CreateLookAtNode
        , CreateLookAtYPosAxisNode
#endif
        >;
} // namespace ScriptCanvas

