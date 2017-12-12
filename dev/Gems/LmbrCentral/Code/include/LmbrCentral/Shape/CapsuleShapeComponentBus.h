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

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /** 
     * Type ID for CapsuleShapeComponent
     */
    static const AZ::Uuid CapsuleShapeComponentTypeId = "{967EC13D-364D-4696-AB5C-C00CC05A2305}";

    /**
     * Type ID for EditorCapsuleShapeComponent
     */
    static const AZ::Uuid EditorCapsuleShapeComponentTypeId = "{06B6C9BE-3648-4DA2-9892-755636EF6E19}";

    /**
     * Configuration data for CapsuleShapeComponent
     */
    class CapsuleShapeConfig
        : public AZ::ComponentConfig
    {
    public:

        AZ_CLASS_ALLOCATOR(CapsuleShapeConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(CapsuleShapeConfig, "{00931AEB-2AD8-42CE-B1DC-FA4332F51501}", ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ_INLINE void SetHeight(float newHeight)
        {
            m_height = newHeight;
        }

        AZ_INLINE float GetHeight() const
        {
            return m_height;
        }

        AZ_INLINE void SetRadius(float newRadius)
        {
            m_radius = newRadius;
        }

        AZ_INLINE float GetRadius() const
        {
            return m_radius;
        }

        //! The end to end height of capsule, this includes the cylinder and both caps
        float m_height = 1.f;

        //! The radius of this capsule
        float m_radius = 0.25f;
    };

    using CapsuleShapeConfiguration = CapsuleShapeConfig; ///< @deprecated Use CapsuleShapeConfig
   

    /*!
    * Services provided by the Capsule Shape Component
    */
    class CapsuleShapeComponentRequests : public AZ::ComponentBus
    {
    public:
        virtual CapsuleShapeConfig GetCapsuleConfiguration() = 0;

        /**
        * \brief Sets the end to end height of capsule, this includes the cylinder and both caps
        * \param newHeight new height of the capsule
        */
        virtual void SetHeight(float newHeight) = 0;

        /**
        * \brief Sets radius of the capsule
        * \param newRadius new radius of the capsule
        */
        virtual void SetRadius(float newRadius) = 0;

    };

    // Bus to service the Capsule Shape component event group
    using CapsuleShapeComponentRequestsBus = AZ::EBus<CapsuleShapeComponentRequests>;

} // namespace LmbrCentral