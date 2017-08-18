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

namespace AZ
{
    class Transform;
    class Vector3;
}

namespace LmbrCentral
{
    namespace Animation
    {
        /**
         * Contains all motion parameters required for blending.
         */
        struct MotionParameters
        {
            AZ_TYPE_INFO(MotionParameters, "{CE25D14E-FB49-4A32-93CF-8381C6AE122D}");

            float m_groundAngleSigned           = 0.f;
            AZ::Vector2 m_relativeTravelDelta   = AZ::Vector2::CreateZero();
            float m_travelDistance              = 0.f;
            float m_travelSpeed                 = 0.f;
            float m_turnAngle                   = 0.f;
            float m_turnSpeed                   = 0.f;
        };

        /**
         * Extract motion parameters for a given frame, for the purpose of driving blends.
         * \param deltaTime Frame delta time (in seconds).
         * \param entityTransform The character/entity's current world transform.
         * \param relativeMotionDelta Character-relative motion (rotation and translation) for the current frame.
         * \param worldGroundNormal The ground contact normal under the character for the current frame, in world space.
         */
        MotionParameters ExtractMotionParameters(float deltaTime, const AZ::Transform& entityTransform, const AZ::Transform& relativeMotionDelta, const AZ::Vector3& worldGroundNormal);

        /**
         * Contains smoothing settings to apply to motion parameters prior to passing ot the blending system.
         */
        struct MotionParameterSmoothingSettings
        {
            AZ_TYPE_INFO(MotionParameterSmoothingSettings, "{7DB44746-EA1D-4A53-9270-7600A5AA8027}");

            float m_groundAngleConvergeTime     = 0.1f;    ///< Approximate time (in seconds) to converge on precise ground angle value.
            float m_travelAngleConvergeTime     = 0.1f;    ///< Approximate time (in seconds) to converge on precise travel angle value.
            float m_travelDistanceConvergeTime  = 0.1f;    ///< Approximate time (in seconds) to converge on precise travel distance value.
            float m_travelSpeedConvergeTime     = 0.1f;    ///< Approximate time (in seconds) to converge on precise travel speed value.
            float m_turnAngleConvergeTime       = 0.2f;    ///< Approximate time (in seconds) to converge on precise turn angle value.
            float m_turnSpeedConvergeTime       = 0.1f;    ///< Approximate time (in seconds) to converge on precise turn speed value.
        };

        /**
         * Represents state required for dampended-spring smoothing of motion paramters.
         * To be used with SmoothMotionParameters, a persistent instance of this structure must be maintained to achieve proper smoothing.
         */
        struct MotionParameterSmoothingState : public MotionParameters
        {
        };

        /**
         * Given a set of motion parameters and a desired smoothing settings, generate a smoothed set of motion parameters.
         * \param smoothedParams Structure containing current output params, and will be updated with next set of smoothed params.
         * \param targetParams Structure containing the desired target output params, to which we're converging.
         * \param state Required state for PD-controller (dampened spring) computation (provides ease in/out behavior).
         * \param deltaTime Frame delta time (in seconds).
         */
        void SmoothMotionParameters(MotionParameters& smoothedParams, const MotionParameters& targetParams, MotionParameterSmoothingState& state, const MotionParameterSmoothingSettings& settings, float deltaTime);

    } // namespace Animation

} // namespace LmbrCentral
