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

#include <Cry_Geo.h>
#include <MathConversion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateCylinderColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    /**
     * Configuration data for CylinderShapeComponent
     */
    class CylinderShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(CylinderShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(CylinderShapeConfiguration, "{53254779-82F1-441E-9116-81E1FACFECF4}");
        static void Reflect(AZ::ReflectContext* context)
        {
            auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                // Deprecate: CylinderColliderConfiguration -> CylinderShapeConfiguration
                serializeContext->ClassDeprecate(
                    "CylinderColliderConfiguration",
                    "{E1DCB833-EFC4-43AC-97B0-4E07AA0DFAD9}",
                    &ClassConverters::DeprecateCylinderColliderConfiguration
                    );

                serializeContext->Class<CylinderShapeConfiguration>()
                    ->Version(1)
                    ->Field("Height", &CylinderShapeConfiguration::m_height)
                    ->Field("Radius", &CylinderShapeConfiguration::m_radius)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<CylinderShapeConfiguration>("Configuration", "Cylinder shape configuration parameters")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &CylinderShapeConfiguration::m_height, "Height", "Height of cylinder")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &CylinderShapeConfiguration::m_radius, "Radius", "Radius of cylinder")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                            ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                            ;
                }
                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<CylinderShapeConfiguration>()
                        ->Property("height", BehaviorValueProperty(&CylinderShapeConfiguration::m_height))
                        ->Property("radius", BehaviorValueProperty(&CylinderShapeConfiguration::m_radius));
                }
            }
        }

        CylinderShapeConfiguration(AZ::ScriptDataContext& context)
        {
            AZ_Warning("Script", context.GetNumArguments() == 2, "Invalid number of arguments to CylinderShapeConfiguration constructor expects (height,radius)");
            AZ_Warning("Script", context.IsNumber(0), "Invalid Parameter , expects (height:number)");
            context.ReadArg(0, m_height);
            AZ_Warning("Script", context.IsNumber(1), "Invalid Parameter , expects (radius:number)");
            context.ReadArg(0, m_radius);
        }

        CylinderShapeConfiguration() {}

        virtual ~CylinderShapeConfiguration() = default;

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

    private:

        //! The height of this cylinder
        float m_height = 1.f;

        //! The radius of this cylinder
        float m_radius = 0.25f;
    };
   

    /*!
    * Services provided by the Cylinder Shape Component
    */
    class CylinderShapeComponentRequests : public AZ::ComponentBus
    {
    public:
        virtual CylinderShapeConfiguration GetCylinderConfiguration() = 0;

        /**
        * \brief Sets height of the cylinder
        * \param newHeight new height of the cylinder
        */
        virtual void SetHeight(float newHeight) = 0;

        /**
        * \brief Sets radius of the cylinder
        * \param newRadius new radius of the cylinder
        */
        virtual void SetRadius(float newRadius) = 0;

    };

    // Bus to service the Cylinder Shape component event group
    using CylinderShapeComponentRequestsBus = AZ::EBus<CylinderShapeComponentRequests>;


    namespace ClassConverters
    {
        static bool DeprecateCylinderColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="CylinderColliderConfiguration" field="Configuration" version="1" type="{E1DCB833-EFC4-43AC-97B0-4E07AA0DFAD9}">
             <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>

            New:
            <Class name="CylinderShapeConfiguration" field="Configuration" version="1" type="{53254779-82F1-441E-9116-81E1FACFECF4}">
             <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            */

            // Cache the Height and Radius
            float oldHeight = 0.f;
            float oldRadius = 0.f;

            int oldIndex = classElement.FindElement(AZ_CRC("Height"));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<float>(oldHeight);
            }

            oldIndex = classElement.FindElement(AZ_CRC("Radius"));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<float>(oldRadius);
            }

            // Convert to CylinderShapeConfiguration
            bool result = classElement.Convert(context, "{53254779-82F1-441E-9116-81E1FACFECF4}");
            if (result)
            {
                int newIndex = classElement.AddElement<float>(context, "Height");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<float>(context, oldHeight);
                }

                newIndex = classElement.AddElement<float>(context, "Radius");
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