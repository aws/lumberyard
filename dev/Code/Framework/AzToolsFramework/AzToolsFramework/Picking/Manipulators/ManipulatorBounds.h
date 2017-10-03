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

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/Picking/BoundInterface.h>

namespace AzToolsFramework
{
    namespace Picking
    {
        class ManipulatorBoundBox
            : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundBox, "{3AD46067-933F-49B4-82E1-DBF12C7BC02E}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundBox, AZ::SystemAllocator, 0);

            ManipulatorBoundBox(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            { }

            ~ManipulatorBoundBox() {};

            bool IntersectRay(const AZ::Vector3 &rayOrigin, const AZ::Vector3 &rayDir, float &t) override;
            void SetShapeData(const BoundRequestShapeBase& shapeData);

            AZ::Vector3 m_center = AZ::Vector3::CreateZero();
            AZ::Vector3 m_axis1 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_axis2 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_axis3 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_halfExtents = AZ::Vector3::CreateZero();
        };

        class ManipulatorBoundCylinder
            : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundCylinder, "{D248F9E4-22E6-41A8-898D-704DF307B533}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundCylinder, AZ::SystemAllocator, 0);

            ManipulatorBoundCylinder(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            { }

            ~ManipulatorBoundCylinder() {};

            bool IntersectRay(const AZ::Vector3 &rayOrigin, const AZ::Vector3 &rayDir, float &t) override;
            void SetShapeData(const BoundRequestShapeBase& shapeData);

            // The center of the circle on one end of the cylinder, the other one m_end2 = m_end1 + m_height * m_dir.
            AZ::Vector3 m_end1 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_dir = AZ::Vector3::CreateZero();
            float m_height = 0.0f;
            float m_radius = 0.0f;
        };

        class ManipulatorBoundCone
            : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundCone, "{9430440D-DFF2-4A60-9073-507C4E9DD65D}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundCone, AZ::SystemAllocator, 0);

            ManipulatorBoundCone(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            { }

            ~ManipulatorBoundCone() {};

            bool IntersectRay(const AZ::Vector3 &rayOrigin, const AZ::Vector3 &rayDir, float &t) override;
            void SetShapeData(const BoundRequestShapeBase& shapeData);

            AZ::Vector3 m_apexPosition = AZ::Vector3::CreateZero();
            AZ::Vector3 m_dir = AZ::Vector3::CreateZero();
            float m_radius = 0.0f;
            float m_height = 0.0f;
        };

        /**
         * The quad shape consists of 4 points in 3D space. Please set them from \ref m_corner1 to \ref m_corner4
         * in either clock-wise winding or counter clock-wise winding. In another word, \ref m_corner1 and
         * \ref corner_2 cannot be diagonal corners.
         */
        class ManipulatorBoundQuad
            : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundQuad, "{3CDED61C-5786-4299-B5F2-5970DE4457AD}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundQuad, AZ::SystemAllocator, 0);

            ManipulatorBoundQuad(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            { }

            ~ManipulatorBoundQuad() {};

            bool IntersectRay(const AZ::Vector3 &rayOrigin, const AZ::Vector3 &rayDir, float &t) override;
            void SetShapeData(const BoundRequestShapeBase& shapeData);

            AZ::Vector3 m_corner1 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_corner2 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_corner3 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_corner4 = AZ::Vector3::CreateZero();
        };

        class ManipulatorBoundTorus
            : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundTorus, "{46E4711C-178A-4F97-BC14-A048D096E7A1}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundTorus, AZ::SystemAllocator, 0);

            ManipulatorBoundTorus(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            { }

            ~ManipulatorBoundTorus() {};

            bool IntersectRay(const AZ::Vector3 &rayOrigin, const AZ::Vector3 &rayDir, float &t) override;
            void SetShapeData(const BoundRequestShapeBase& shapeData);

            // Approximate a torus as a thin cylinder. A ray intersects a torus when the ray and the torus's
            // approximating cylinder have an intersecting point that is at certain distance away from the 
            // center of the torus.
            AZ::Vector3 m_end1 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_dir = AZ::Vector3::CreateZero();
            float m_height = 0.0f;
            float m_radius = 0.0f;
        };
    } // namespace Picking
} // namespace AzToolsFramework