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
        static bool DeprecateCapsuleColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    /**
     * Configuration data for CapsuleShapeComponent
     */
    class CapsuleShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(CapsuleShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(CapsuleShapeConfiguration, "{00931AEB-2AD8-42CE-B1DC-FA4332F51501}");
        static void Reflect(AZ::ReflectContext* context)
        {
            auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                // Deprecate: CapsuleColliderConfiguration -> CapsuleShapeConfiguration
                serializeContext->ClassDeprecate(
                    "CapsuleColliderConfiguration",
                    "{902BCDA9-C9E5-429C-991B-74C241ED2889}",
                    &ClassConverters::DeprecateCapsuleColliderConfiguration
                    );

                serializeContext->Class<CapsuleShapeConfiguration>()
                    ->Version(1)
                    ->Field("Height", &CapsuleShapeConfiguration::m_height)
                    ->Field("Radius", &CapsuleShapeConfiguration::m_radius)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<CapsuleShapeConfiguration>("Configuration", "Capsule shape configuration parameters")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShapeConfiguration::m_height, "Height", "End to end height of capsule, this includes the cylinder and both caps")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CapsuleShapeConfiguration::OnConfigurationChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShapeConfiguration::m_radius, "Radius", "Radius of capsule")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                            ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CapsuleShapeConfiguration::OnConfigurationChanged)
                            ;
                }
                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<CapsuleShapeConfiguration>()
                        ->Property("height", BehaviorValueProperty(&CapsuleShapeConfiguration::m_height))
                        ->Property("radius", BehaviorValueProperty(&CapsuleShapeConfiguration::m_radius));
                }
            }
        }

        CapsuleShapeConfiguration(AZ::ScriptDataContext& context)
        {
            AZ_Warning("Script", context.GetNumArguments() == 2, "Invalid number of arguments to CapsuleShapeConfiguration constructor expects (height,radius)");
            AZ_Warning("Script", context.IsNumber(0), "Invalid Parameter , expects (height:number)");
            context.ReadArg(0, m_height);
            AZ_Warning("Script", context.IsNumber(1), "Invalid Parameter , expects (radius:number)");
            context.ReadArg(0, m_radius);
        }

        CapsuleShapeConfiguration() {}

        virtual ~CapsuleShapeConfiguration() = default;

        AZ_INLINE void SetHeight(float newHeight)
        {
            m_height = newHeight;
            OnConfigurationChanged();
        }

        AZ_INLINE float GetHeight() const
        {
            return m_height;
        }

        AZ_INLINE void SetRadius(float newRadius)
        {
            m_radius = newRadius;
            OnConfigurationChanged();
        }

        AZ_INLINE float GetRadius() const
        {
            return m_radius;
        }

        AZ_INLINE AZ::u32 OnConfigurationChanged()
        {
            if ((m_height - 2 * m_radius) < std::numeric_limits<float>::epsilon())
            {
                m_height = 2 * m_radius;
                return AZ_CRC("RefreshValues");
            }

            return AZ_CRC("RefreshNone");
        }
    private:

        //! The end to end height of capsule, this includes the cylinder and both caps
        float m_height = 1.f;

        //! The radius of this capsule
        float m_radius = 0.25f;
    };
   

    /*!
    * Services provided by the Capsule Shape Component
    */
    class CapsuleShapeComponentRequests : public AZ::ComponentBus
    {
    public:
        virtual CapsuleShapeConfiguration GetCapsuleConfiguration() = 0;

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


    namespace ClassConverters
    {
        static bool DeprecateCapsuleColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="CapsuleColliderConfiguration" field="Configuration" version="1" type="{902BCDA9-C9E5-429C-991B-74C241ED2889}">
             <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>

            New:
            <Class name="CapsuleShapeConfiguration" field="Configuration" version="1" type="{00931AEB-2AD8-42CE-B1DC-FA4332F51501}">
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

            // Convert to CapsuleShapeConfiguration
            bool result = classElement.Convert(context, "{00931AEB-2AD8-42CE-B1DC-FA4332F51501}");
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