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
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateBoxColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    /**
     * Configuration data for BoxShapeComponent
     */
    class BoxShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(BoxShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(BoxShapeConfiguration, "{F034FBA2-AC2F-4E66-8152-14DFB90D6283}");

        BoxShapeConfiguration(AZ::Vector3& dimensions)
        {
            m_dimensions = dimensions;
        }

        BoxShapeConfiguration(){}

        static void Reflect(AZ::ReflectContext* context)
        {
            auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                // Deprecate: BoxColliderConfiguration -> BoxShapeConfiguration
                serializeContext->ClassDeprecate(
                    "BoxColliderConfiguration",
                    "{282E47CB-9F6D-47AE-A210-4CE879527EFD}",
                    &ClassConverters::DeprecateBoxColliderConfiguration
                    );

                serializeContext->Class<BoxShapeConfiguration>()
                    ->Version(1)
                    ->Field("Dimensions", &BoxShapeConfiguration::m_dimensions)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<BoxShapeConfiguration>("Configuration", "Box shape configuration parameters")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly"))
                        ->DataElement(0, &BoxShapeConfiguration::m_dimensions, "Dimensions", "Dimensions of the box along its axes")
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                            ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                            ;
                }

                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<BoxShapeConfiguration>()
                        ->Constructor<AZ::Vector3&>()
                        ->Property("dimensions", BehaviorValueProperty(&BoxShapeConfiguration::m_dimensions));
                }
            }
        }

        virtual ~BoxShapeConfiguration() = default;

        AZ_INLINE void SetDimensions(const AZ::Vector3& newDimensions)
        {
            m_dimensions = newDimensions;
        }

        AZ_INLINE AZ::Vector3 GetDimensions() const
        {
            return m_dimensions;
        }
    private:

        //! Stores the dimensions of the box along each axis
        AZ::Vector3 m_dimensions = AZ::Vector3(1.f, 1.f, 1.f);
    };
   

    /*!
    * Services provided by the Box Shape Component
    */
    class BoxShapeComponentRequests : public AZ::ComponentBus
    {
    public:
        virtual BoxShapeConfiguration GetBoxConfiguration() = 0;

        /**
        * \brief Sets new dimensions for the Box Shape
        * \param newDimensions Vector3 indicating new dimensions along the x,y & z axis
        */
        virtual void SetBoxDimensions(AZ::Vector3 newDimensions) = 0;
    };

    // Bus to service the Box Shape component event group
    using BoxShapeComponentRequestsBus = AZ::EBus<BoxShapeComponentRequests>;


    namespace ClassConverters
    {
        static bool DeprecateBoxColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="BoxColliderConfiguration" field="Configuration" version="1" type="{282E47CB-9F6D-47AE-A210-4CE879527EFD}">
             <Class name="Vector3" field="Size" value="1.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
            </Class>

            New:
            <Class name="BoxShapeConfiguration" field="Configuration" version="1" type="{F034FBA2-AC2F-4E66-8152-14DFB90D6283}">
             <Class name="Vector3" field="Dimensions" value="1.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
            </Class>
            */

            // Cache the Dimensions
            AZ::Vector3 oldDimensions;
            int oldIndex = classElement.FindElement(AZ_CRC("Size"));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<AZ::Vector3>(oldDimensions);
            }

            // Convert to BoxShapeConfiguration
            bool result = classElement.Convert(context, "{F034FBA2-AC2F-4E66-8152-14DFB90D6283}");
            if (result)
            {
                int newIndex = classElement.AddElement<AZ::Vector3>(context, "Dimensions");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<AZ::Vector3>(context, oldDimensions);
                    return true;
                }
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral