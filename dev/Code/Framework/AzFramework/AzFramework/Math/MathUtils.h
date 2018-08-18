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

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>

#ifndef AZFRAMEWORK_MATHUTILS_H
#define AZFRAMEWORK_MATHUTILS_H

namespace AzFramework
{
    //! \brief Converts a transform to corresponding component-wise Euler angles.
    //! \param Transform transform The input transform to decompose.
    //! \return Vector3 A vector containing component-wise rotation angles in degrees.
    // Technique from published work available here
    // https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2012/07/euler-angles1.pdf (Extracting Euler Angles from a Rotation Matrix - Mike Day, Insomniac Games mday@insomniacgames.com)
    AZ_DEPRECATED(AZ::Vector3 ConvertTransformToEulerDegrees(const AZ::Transform& transform), "Function deprecated, use AZ::ConvertTransformToEulerDegrees instead.");
    AZ_DEPRECATED(AZ::Vector3 ConvertTransformToEulerRadians(const AZ::Transform& transform), "Function deprecated, use AZ::ConvertTransformToEulerRadians instead.");

    //! \brief Create a transform from Euler Angles (e.g. rotation angles in X, Y, and Z)
    //! \param Vector3 eulerDegrees A vector containing component-wise rotation angles in degrees.
    //! \return Transform A transform made from the rotational components.
    AZ_DEPRECATED(AZ::Transform ConvertEulerDegreesToTransform(const AZ::Vector3& eulerDegrees), "Function deprecated, use AZ::ConvertEulerDegreesToTransform instead.");

    //! \brief Create a rotation transform from Euler angles in radian around each base axis.
    //!        This version uses precise sin/cos for a more accurate conversion.
    //! \param Vector3 eulerDegrees A vector containing component-wise rotation angles in radian.
    //! \return Transform A transform made from the composite of rotations first around z-axis, and y-axis and then x-axis.
    AZ_DEPRECATED(AZ::Transform ConvertEulerRadiansToTransformPrecise(const AZ::Vector3& eulerRadians), "Function deprecated, use AZ::ConvertEulerRadiansToTransformPrecise instead.");

    //! \brief Create a rotation transform from Euler angles in degree around each base axis.
    //!        This version uses precise sin/cos for a more accurate conversion.
    //! \param Vector3 eulerDegrees A vector containing component-wise rotation angles in degree.
    //! \return Transform A transform made from the composite of rotations first around z-axis, and y-axis and then x-axis.
    AZ_DEPRECATED(AZ::Transform ConvertEulerDegreesToTransformPrecise(const AZ::Vector3& eulerDegrees), "Function deprecated, use AZ::ConvertEulerDegreesToTransformPrecise instead.");

    //! \brief Create, from a quaternion, a set of Euler angles of rotations around first z-axis,
    //!        then y-axis and then x-axis.
    //! \param Quaternion a quaternion representing the rotation
    //! \return Vector3 A vector containing component-wise rotation angles in degrees.
    AZ_DEPRECATED(AZ::Vector3 ConvertQuaternionToEulerDegrees(const AZ::Quaternion& q), "Function deprecated, use AZ::ConvertQuaternionToEulerDegrees instead.");

    //! \brief Create, from a quaternion, a set of Euler angles of rotations around first z-axis,
    //!        then y-axis and then x-axis.
    //! \param Quaternion a quaternion representing the rotation
    //! \return Vector3 A vector containing component-wise rotation angles in radians.
    AZ_DEPRECATED(AZ::Vector3 ConvertQuaternionToEulerRadians(const AZ::Quaternion& q), "Function deprecated, use AZ::ConvertQuaternionToEulerRadians instead.");

    //! \brief Create a quaternion from composite rotations of Euler angles in the order of 
    //!        rotation around first z-axis, then y-axis and then x-axis.
    //! \param Vector3 eulerRadians A vector containing component-wise rotation angles in radians.
    //! \return Quaternion a quaternion made from composition of rotations around principle axes.
    AZ_DEPRECATED(AZ::Quaternion ConvertEulerRadiansToQuaternion(const AZ::Vector3& eulerRadians), "Function deprecated, use AZ::ConvertEulerRadiansToQuaternion instead.");

    //! \brief Create a quaternion from composite rotations of Euler angles in the order of 
    //!        rotation around first z-axis, then y-axis and then x-axis.
    //! \param Vector3 eulerDegrees A vector containing component-wise rotation angles in degrees.
    //! \return Quaternion a quaternion made from composition of rotations around principle axes.
    AZ_DEPRECATED(AZ::Quaternion ConvertEulerDegreesToQuaternion(const AZ::Vector3& eulerDegrees), "Function deprecated, use AZ::ConvertEulerDegreesToQuaternion instead.");

    //! \brief Populate axis and angle of rotation from Quaternion
    //! \param Quaternion quat A source quaternion
    //! \param Vector3 axis A Vector3 defining the rotation axis.
    //! \param float angle A float rotation angle around the axis in radians.
    AZ_DEPRECATED(void ConvertQuaternionToAxisAngle(const AZ::Quaternion& quat, AZ::Vector3& outAxis, float& OutAngle), "Function deprecated, use AZ::ConvertQuaternionToAxisAngle instead.");

    //! Degrees-Radians conversions on AZ::Vector3
    AZ_DEPRECATED( AZ_MATH_FORCE_INLINE AZ::Vector3 RadToDeg(const AZ::Vector3& radians)
    {
        return radians * 180.f / AZ::Constants::Pi;
    }, "Function deprecated, use AZ::Vector3RadToDeg instead.");

    AZ_DEPRECATED( AZ_MATH_FORCE_INLINE AZ::Vector3 DegToRad(const AZ::Vector3& degrees)
    {
        return degrees * AZ::Constants::Pi / 180.f;
    }, "Function deprecated, use AZ::Vector3DegToRad instead.");


    enum class Axis : AZ::u8
    {
        XPositive,
        XNegative,
        YPositive,
        YNegative,
        ZPositive,
        ZNegative
    };

    //! \brief Create a "look-at" transform, given a source position and target position,
    //! make a transform at the source position that points toward the target along a chosen
    //! local-space axis.
    //! \param Vector3 from The source position (world-space).
    //! \param Vector3 to The target position (world-space).
    //! \param Axis forwardAxis The local-space basis axis that should "look-at" the target.
    //! \return Transform The resulting transform, or identity transform if from == to.
    AZ_DEPRECATED(AZ::Transform CreateLookAt(const AZ::Vector3& from, const AZ::Vector3& to, Axis forwardAxis = Axis::YPositive), "Function deprecated, use AZ::Transform::CreateLookAt instead.");

} // namespace AzFramework

#endif // AZFRAMEWORK_MATHUTILS_H
