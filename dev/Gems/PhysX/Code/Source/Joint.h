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

#include <AzFramework/Physics/Joint.h>

namespace PhysX
{
    class D6JointLimitConfiguration
        : public Physics::JointLimitConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(D6JointLimitConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(D6JointLimitConfiguration, "{90C5C23D-16C0-4F23-AD50-A190E402388E}", Physics::JointLimitConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        const char* GetTypeName() override;

        float m_swingLimitY = 45.0f; ///< Maximum angle in degrees from the Y axis of the joint frame.
        float m_swingLimitZ = 45.0f; ///< Maximum angle in degrees from the Z axis of the joint frame.
        float m_twistLimitLower = -45.0f; ///< Lower limit in degrees for rotation about the X axis of the joint frame.
        float m_twistLimitUpper = 45.0f; ///< Upper limit in degrees for rotation about the X axis of the joint frame.
    };

    class Joint
        : public Physics::Joint
    {
    public:
        AZ_CLASS_ALLOCATOR(Joint, AZ::SystemAllocator, 0);
        AZ_RTTI(Joint, "{3C739E22-8EF0-419F-966B-C575A1F5A08B}", Physics::Joint);

        Joint(physx::PxJoint* pxJoint, Physics::WorldBody* parentBody,
            Physics::WorldBody* childBody);

        Physics::WorldBody* GetParentBody() const override;
        Physics::WorldBody* GetChildBody() const override;
        void SetParentBody(Physics::WorldBody* parentBody) override;
        void SetChildBody(Physics::WorldBody* childBody) override;
        const AZStd::string& GetName() const override;
        void SetName(const AZStd::string& name) override;
        void* GetNativePointer() override;

    protected:
        bool SetPxActors();

        using PxJointUniquePtr = AZStd::unique_ptr<physx::PxJoint, AZStd::function<void(physx::PxJoint*)>>;
        PxJointUniquePtr m_pxJoint;
        Physics::WorldBody* m_parentBody;
        Physics::WorldBody* m_childBody;
        AZStd::string m_name;
    };

    class D6Joint
        : public Joint
    {
    public:
        AZ_CLASS_ALLOCATOR(D6Joint, AZ::SystemAllocator, 0);
        AZ_RTTI(D6Joint, "{962C4044-2BD2-4E4C-913C-FB8E85A2A12A}", Joint);

        D6Joint(physx::PxJoint* pxJoint, Physics::WorldBody* parentBody,
            Physics::WorldBody* childBody)
            : Joint(pxJoint, parentBody, childBody)
        {
        }
        virtual ~D6Joint() = default;

        const AZ::Crc32 GetNativeType() const override;
        void GenerateJointLimitVisualizationData(
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut) override;
    };

    struct D6JointState
    {
        float m_swingAngleY;
        float m_swingAngleZ;
        float m_twistAngle;
    };

    class JointUtils
    {
    public:
        static AZStd::vector<AZ::TypeId> GetSupportedJointTypes();

        static AZStd::shared_ptr<Physics::JointLimitConfiguration> CreateJointLimitConfiguration(AZ::TypeId jointType);

        static AZStd::shared_ptr<Physics::Joint> CreateJoint(const AZStd::shared_ptr<Physics::JointLimitConfiguration>& configuration,
            Physics::WorldBody* parentBody, Physics::WorldBody* childBody);

        static D6JointState CalculateD6JointState(
            const AZ::Quaternion& parentWorldRotation,
            const AZ::Quaternion& parentLocalRotation,
            const AZ::Quaternion& childWorldRotation,
            const AZ::Quaternion& childLocalRotation);

        static bool IsD6SwingValid(
            float swingAngleY,
            float swingAngleZ,
            float swingLimitY,
            float swingLimitZ);

        static void AppendD6SwingConeToLineBuffer(
            const AZ::Quaternion& parentLocalRotation,
            float swingAngleY,
            float swingAngleZ,
            float swingLimitY,
            float swingLimitZ,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut);

        static void AppendD6TwistArcToLineBuffer(
            const AZ::Quaternion& parentLocalRotation,
            float twistAngle,
            float twistLimitLower,
            float twistLimitUpper,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut);

        static void AppendD6CurrentTwistToLineBuffer(
            const AZ::Quaternion& parentLocalRotation,
            float twistAngle,
            float twistLimitLower,
            float twistLimitUpper,
            float scale,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut);

        static void GenerateJointLimitVisualizationData(
            const Physics::JointLimitConfiguration& configuration,
            const AZ::Quaternion& parentRotation,
            const AZ::Quaternion& childRotation,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut);

        static AZStd::unique_ptr<Physics::JointLimitConfiguration> ComputeInitialJointLimitConfiguration(
            const AZ::TypeId& jointLimitTypeId,
            const AZ::Quaternion& parentWorldRotation,
            const AZ::Quaternion& childWorldRotation,
            const AZ::Vector3& axis,
            const AZStd::vector<AZ::Quaternion>& exampleLocalRotations);
    };
} // namespace PhysX
