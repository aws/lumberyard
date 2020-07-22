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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/function/function_template.h>

#include <System/DataTypes.h>

namespace NvCloth
{
    //! Configuration data for Cloth.
    struct ClothConfiguration
    {
        AZ_CLASS_ALLOCATOR(ClothConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(ClothConfiguration, "{96E2AF5E-3C98-4872-8F90-F56302A44F2A}");

        static void Reflect(AZ::ReflectContext* context);

        virtual ~ClothConfiguration() = default;

        AZStd::string m_meshNode;

        // Mass and Gravity parameters
        float m_mass = 1.0f;
        bool m_useCustomGravity = false;
        AZ::Vector3 m_customGravity = AZ::Vector3(0.0f, 0.0f, -9.81f);
        float m_gravityScale = 1.0f;

        // Blend factor of cloth simulation with authored skinning animation
        // 0 - Cloth mesh fully simulated
        // 1 - Cloth mesh fully animated
        float m_animationBlendFactor = 0.0f;

        // Global stiffness frequency
        float m_stiffnessFrequency = 10.0f;

        // Damping parameters
        AZ::Vector3 m_damping = AZ::Vector3(0.2f, 0.2f, 0.2f);
        AZ::Vector3 m_linearDrag = AZ::Vector3(0.2f, 0.2f, 0.2f);
        AZ::Vector3 m_angularDrag = AZ::Vector3(0.2f, 0.2f, 0.2f);

        // Inertia parameters
        AZ::Vector3 m_linearInteria = AZ::Vector3::CreateOne();
        AZ::Vector3 m_angularInteria = AZ::Vector3::CreateOne();
        AZ::Vector3 m_centrifugalInertia = AZ::Vector3::CreateOne();

        // Wind parameters
        AZ::Vector3 m_windVelocity = AZ::Vector3(0.0f, 20.0f, 0.0f);
        float m_airDragCoefficient = 0.0f;
        float m_airLiftCoefficient = 0.0f;
        float m_fluidDensity = 1.0f;

        // Collision parameters
        float m_collisionFriction = 0.0f;
        float m_collisionMassScale = 0.0f;
        bool m_continuousCollisionDetection = false;

        // Self Collision parameters
        float m_selfCollisionDistance = 0.0f;
        float m_selfCollisionStiffness = 0.2f;

        // Tether Constraints parameters
        float m_tetherConstraintStiffness = 1.0f;
        float m_tetherConstraintScale = 1.0f;

        // Quality parameters
        float m_solverFrequency = 300.0f;
        uint32_t m_accelerationFilterIterations = 30;

        // Fabric phases parameters
        float m_horizontalStiffness = 1.0f;
        float m_horizontalStiffnessMultiplier = 0.0f;
        float m_horizontalCompressionLimit = 0.0f;
        float m_horizontalStretchLimit = 0.0f;
        float m_verticalStiffness = 1.0f;
        float m_verticalStiffnessMultiplier = 0.0f;
        float m_verticalCompressionLimit = 0.0f;
        float m_verticalStretchLimit = 0.0f;
        float m_bendingStiffness = 1.0f;
        float m_bendingStiffnessMultiplier = 0.0f;
        float m_bendingCompressionLimit = 0.0f;
        float m_bendingStretchLimit = 0.0f;
        float m_shearingStiffness = 1.0f;
        float m_shearingStiffnessMultiplier = 0.0f;
        float m_shearingCompressionLimit = 0.0f;
        float m_shearingStretchLimit = 0.0f;

    private:
        // Making private functionality related with the Editor Context reflection,
        // it's unnecessary for the clients using ClothConfiguration.
        friend class EditorClothComponent;

        // Callback function set by the EditorClothComponent to gather the list of mesh nodes,
        // showed to the user in a combo box for the m_meshNode member.
        AZStd::function<MeshNodeList()> m_populateMeshNodeListCallback;

        // Used in the StringList attribute of the combobox for m_meshNode member.
        // The StringList attribute doesn't work with the AZStd::function directly, so
        // this function will just call m_populateMeshNodeListCallback if it has been set.
        MeshNodeList PopulateMeshNodeList();

        // Callback function set by the EditorClothComponent to show animation blend parameter.
        AZStd::function<bool()> m_showAnimationBlendFactorCallback;

        // Used in the Visible attribute of m_animationBlendFactor member.
        // The Visible attribute doesn't work with the AZStd::function directly, so
        // this function will just call m_showAnimationBlendFactorCallback if it has been set.
        bool ShowAnimationBlendFactor();

        // Callback function set by the EditorClothComponent to provide the entity id to the combobox widget.
        AZStd::function<AZ::EntityId()> m_getEntityIdCallback;

        // Used in the EntityId attribute of the combobox for m_meshNode member.
        // The EntityId attribute doesn't work with the AZStd::function directly, so
        // this function will just call m_getEntityIdCallback if it has been set.
        AZ::EntityId GetEntityId();
    };
} // namespace NvCloth
