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
#include <AzCore/RTTI/RTTI.h>

namespace AzToolsFramework
{
    namespace Picking
    {
        using RegisteredBoundId = AZ::u64;
        class BoundRequestShapeBase;

        /** 
         * This class serves as the base class for the actual bound shapes that various DefaultContextBoundManager-derived 
         * classes return from the function CreateShape.
         */
        class BoundShapeInterface
        {
        public:
            AZ_RTTI(BoundShapeInterface, "{C639CB8E-1957-4E4F-B889-3BE1DFBC358D}");

            explicit BoundShapeInterface(RegisteredBoundId boundId, AZ::u64 userContext = 0) 
                : m_boundId(boundId)
                , m_userContext(userContext) 
                , m_valid(false)
            {}

            virtual ~BoundShapeInterface() = default;

            RegisteredBoundId GetBoundId() const { return m_boundId; }
            AZ::u64 GetUserContext() const { return m_userContext; }

            /**
             * @param      rayOrigin The origin of the ray to test with.
             * @param      rayDir    The direction of the ray to test with.
             * @param[out] t         The coefficient of the intersecting point closest to the ray origin.
             * @return Boolean indicating whether there is a least one intersecting point between this bound shape and the ray.
             */
            virtual bool IntersectRay(const AZ::Vector3 &/*rayOrigin*/, const AZ::Vector3 &/*rayDir*/, float &t) { t = AZ::g_fltMax; return false; }

            virtual void SetShapeData(const BoundRequestShapeBase& shapeData) = 0;

            void SetValidity(bool valid) { m_valid = valid; }
            bool IsValid() const { return m_valid; }

        private:
            RegisteredBoundId m_boundId;
            AZ::u64 m_userContext;
            bool m_valid;
        };
    } // namespace Picking
} // namespace AzToolsFramework
