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

#include "ShapeComponentBus.h"
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateSphereColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    /**
     * Configuration data for SphereShapeComponent
     */
    class SphereShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(SphereShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(SphereShapeConfiguration, "{4AADFD75-48A7-4F31-8F30-FE4505F09E35}");
        static void Reflect(AZ::ReflectContext* context)
        {
            auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                // Deprecate: SphereColliderConfiguration -> SphereShapeConfiguration
                serializeContext->ClassDeprecate(
                    "SphereColliderConfiguration",
                    "{0319AE62-3355-4C98-873D-3139D0427A53}",
                    &ClassConverters::DeprecateSphereColliderConfiguration
                    );

                serializeContext->Class<SphereShapeConfiguration>()
                    ->Version(1)
                    ->Field("Radius", &SphereShapeConfiguration::m_radius)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SphereShapeConfiguration>("Configuration", "Sphere shape configuration parameters")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly"))
                        ->DataElement(0, &SphereShapeConfiguration::m_radius, "Radius", "Radius of sphere")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                            ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                            ;
                }

                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<SphereShapeConfiguration>()
                        ->Constructor<float>()
                        ->Property("radius", BehaviorValueProperty(&SphereShapeConfiguration::m_radius));
                }
            }
        }

        virtual ~SphereShapeConfiguration() = default;

        SphereShapeConfiguration() {}

        SphereShapeConfiguration(float radius)
        {
            m_radius = radius;
        }

        AZ_INLINE void SetRadius(float newRadius)
        {
            m_radius = newRadius;
        }

        AZ_INLINE float GetRadius() const
        {
            return m_radius;
        }

    private:

        float m_radius = 1.f;
    };
   

    /*!
    * Services provided by the Sphere Shape Component
    */
    class SphereShapeComponentRequests : public AZ::ComponentBus
    {
    public:
        virtual SphereShapeConfiguration GetSphereConfiguration() = 0;

        /**
        * \brief Sets the radius for the sphere shape component
        * \param new Radius of the sphere shape 
        */
        virtual void SetRadius(float newRadius) = 0;
    };

    // Bus to service the Sphere Shape component event group
    using SphereShapeComponentRequestsBus = AZ::EBus<SphereShapeComponentRequests>;


    namespace ClassConverters
    {
        static bool DeprecateSphereColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="SphereColliderConfiguration" field="Configuration" version="1" type="{0319AE62-3355-4C98-873D-3139D0427A53}">
             <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>

            New:
            <Class name="SphereShapeConfiguration" field="Configuration" version="1" type="{4AADFD75-48A7-4F31-8F30-FE4505F09E35}">
             <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            */

            // Cache the Radius
            float oldRadius = 0.f;
            int oldIndex = classElement.FindElement(AZ_CRC("Radius"));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<float>(oldRadius);
            }

            // Convert to SphereShapeConfiguration
            bool result = classElement.Convert(context, "{4AADFD75-48A7-4F31-8F30-FE4505F09E35}");
            if (result)
            {
                int newIndex = classElement.AddElement<float>(context, "Radius");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<float>(context, oldRadius);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral