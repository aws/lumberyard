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

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <LegacyEntityConversion/LegacyEntityConversionBus.h>
#include <OccluderAreaComponentBus.h>
#include "OccluderAreaComponent.h"

namespace Visibility
{
    class OccluderAreaConverter;
    class EditorOccluderAreaComponent;

    class EditorOccluderAreaConfiguration
        : public OccluderAreaConfiguration
    {
    public:
        friend class EditorOccluderAreaComponent; //So it can access component
        AZ_TYPE_INFO(EditorOccluderAreaConfiguration, "{032F466F-25CB-5460-AC2F-B04236C87878}", OccluderAreaConfiguration);
        AZ_CLASS_ALLOCATOR(EditorOccluderAreaConfiguration, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        void OnChange() override;
        void OnVerticesChange() override;

    private:
        EditorOccluderAreaComponent* m_component = nullptr;
    };

    class EditorOccluderAreaComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private OccluderAreaRequestBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
        friend class OccluderAreaConverter;
        friend class EditorOccluderAreaConfiguration;

        using Base = AzToolsFramework::Components::EditorComponentBase;

    public:

        AZ_COMPONENT(EditorOccluderAreaComponent, "{1A209C7C-6C06-5AE6-AD60-22CD8D0DAEE3}", AzToolsFramework::Components::EditorComponentBase);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        static void Reflect(AZ::ReflectContext* context);

        IVisArea* m_area = nullptr;

        EditorOccluderAreaComponent();
        ~EditorOccluderAreaComponent();

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        AZ::Aabb GetEditorSelectionBounds() override;

        /// Update the object runtime after changes to the Configuration.
        /// Called by the default RequestBus SetXXX implementations,
        /// and used to initially set up the object the first time the
        /// Configuration are set.
        void UpdateObject();

        void SetDisplayFilled(const bool value) override
        {
            m_config.m_displayFilled = value;
            UpdateObject();
        }
        bool GetDisplayFilled() override
        {
            return m_config.m_displayFilled;
        }

        void SetCullDistRatio(const float value) override
        {
            m_config.m_cullDistRatio = value;
            UpdateObject();
        }
        float GetCullDistRatio() override
        {
            return m_config.m_cullDistRatio;
        }

        void SetUseInIndoors(const bool value) override
        {
            m_config.m_useInIndoors = value;
            UpdateObject();
        }
        bool GetUseInIndoors() override
        {
            return m_config.m_useInIndoors;
        }

        void SetDoubleSide(const bool value) override
        {
            m_config.m_doubleSide = value;
            UpdateObject();
        }
        bool GetDoubleSide() override
        {
            return m_config.m_doubleSide;
        }

        void DisplayEntity(bool& handled) override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
		AZ_DISABLE_COPY_MOVE(EditorOccluderAreaComponent)

        friend EditorOccluderAreaConfiguration;

        // EntitySelectionEventsBus
        void OnSelected() override;
        void OnDeselected() override;

        // Manipulator handling
        void CreateManipulators();

        //Reflected members
        EditorOccluderAreaConfiguration m_config;

        AzToolsFramework::EditorVertexSelectionFixed<AZ::Vector3> m_vertexSelection; ///< Handles all manipulator interactions with vertices.
    };

    class OccluderAreaConverter: public AZ::LegacyConversion::LegacyConversionEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(OccluderAreaConverter, AZ::SystemAllocator, 0);

        OccluderAreaConverter() {
        }

        // ------------------- LegacyConversionEventBus::Handler -----------------------------
        AZ::LegacyConversion::LegacyConversionResult ConvertEntity(CBaseObject* pEntityToConvert) override;
        bool BeforeConversionBegins() override;
        bool AfterConversionEnds() override;
        AZ::LegacyConversion::LegacyConversionResult ConvertArea(CBaseObject* pEntityToConvert);
        AZ::LegacyConversion::LegacyConversionResult ConvertPlane(CBaseObject* pEntityToConvert);
        // END ----------------LegacyConversionEventBus::Handler ------------------------------
    };
} // namespace Visibility
