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

#include <NvCloth_precompiled.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <System/ClothConfiguration.h>

namespace NvCloth
{
    void ClothConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ClothConfiguration>()
                ->Version(0)
                ->Field("Mesh Node", &ClothConfiguration::m_meshNode)
                ->Field("Mass", &ClothConfiguration::m_mass)
                ->Field("Use Custom Gravity", &ClothConfiguration::m_useCustomGravity)
                ->Field("Custom Gravity", &ClothConfiguration::m_customGravity)
                ->Field("Gravity Scale", &ClothConfiguration::m_gravityScale)
                ->Field("Animation Blend Factor", &ClothConfiguration::m_animationBlendFactor)
                ->Field("Stiffness Frequency", &ClothConfiguration::m_stiffnessFrequency)
                ->Field("Damping", &ClothConfiguration::m_damping)
                ->Field("Linear Drag", &ClothConfiguration::m_linearDrag)
                ->Field("Angular Drag", &ClothConfiguration::m_angularDrag)
                ->Field("Linear Inertia", &ClothConfiguration::m_linearInteria)
                ->Field("Angular Inertia", &ClothConfiguration::m_angularInteria)
                ->Field("Centrifugal Inertia", &ClothConfiguration::m_centrifugalInertia)
                ->Field("Wind Velocity", &ClothConfiguration::m_windVelocity)
                ->Field("Air Drag Coefficient", &ClothConfiguration::m_airDragCoefficient)
                ->Field("Air Lift Coefficient", &ClothConfiguration::m_airLiftCoefficient)
                ->Field("Fluid Density", &ClothConfiguration::m_fluidDensity)
                ->Field("Collision Friction", &ClothConfiguration::m_collisionFriction)
                ->Field("Collision Mass Scale", &ClothConfiguration::m_collisionMassScale)
                ->Field("Continuous Collision Detection", &ClothConfiguration::m_continuousCollisionDetection)
                ->Field("Self Collision Distance", &ClothConfiguration::m_selfCollisionDistance)
                ->Field("Self Collision Stiffness", &ClothConfiguration::m_selfCollisionStiffness)
                ->Field("Horizontal Stiffness", &ClothConfiguration::m_horizontalStiffness)
                ->Field("Horizontal Stiffness Multiplier", &ClothConfiguration::m_horizontalStiffnessMultiplier)
                ->Field("Horizontal Compression Limit", &ClothConfiguration::m_horizontalCompressionLimit)
                ->Field("Horizontal Stretch Limit", &ClothConfiguration::m_horizontalStretchLimit)
                ->Field("Vertical Stiffness", &ClothConfiguration::m_verticalStiffness)
                ->Field("Vertical Stiffness Multiplier", &ClothConfiguration::m_verticalStiffnessMultiplier)
                ->Field("Vertical Compression Limit", &ClothConfiguration::m_verticalCompressionLimit)
                ->Field("Vertical Stretch Limit", &ClothConfiguration::m_verticalStretchLimit)
                ->Field("Bending Stiffness", &ClothConfiguration::m_bendingStiffness)
                ->Field("Bending Stiffness Multiplier", &ClothConfiguration::m_bendingStiffnessMultiplier)
                ->Field("Bending Compression Limit", &ClothConfiguration::m_bendingCompressionLimit)
                ->Field("Bending Stretch Limit", &ClothConfiguration::m_bendingStretchLimit)
                ->Field("Shearing Stiffness", &ClothConfiguration::m_shearingStiffness)
                ->Field("Shearing Stiffness Multiplier", &ClothConfiguration::m_shearingStiffnessMultiplier)
                ->Field("Shearing Compression Limit", &ClothConfiguration::m_shearingCompressionLimit)
                ->Field("Shearing Stretch Limit", &ClothConfiguration::m_shearingStretchLimit)
                ->Field("Tether Constraint Stiffness", &ClothConfiguration::m_tetherConstraintStiffness)
                ->Field("Tether Constraint Scale", &ClothConfiguration::m_tetherConstraintScale)
                ->Field("Solver Frequency", &ClothConfiguration::m_solverFrequency)
                ->Field("Acceleration Filter Iterations", &ClothConfiguration::m_accelerationFilterIterations)
                ;
        }
    }

    MeshNodeList ClothConfiguration::PopulateMeshNodeList()
    {
        if (m_populateMeshNodeListCallback)
        {
            return m_populateMeshNodeListCallback();
        }
        return {};
    }

    bool ClothConfiguration::ShowAnimationBlendFactor()
    {
        if (m_showAnimationBlendFactorCallback)
        {
            return m_showAnimationBlendFactorCallback();
        }
        return false;
    }

    AZ::EntityId ClothConfiguration::GetEntityId()
    {
        if (m_getEntityIdCallback)
        {
            return m_getEntityIdCallback();
        }
        return AZ::EntityId();
    }
} // namespace NvCloth
