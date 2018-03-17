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

#include <AzCore/Math/Color.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <LegacyEntityConversion/LegacyEntityConversionBus.h>

#include "VisAreaComponent.h"

namespace Visibility 
{
    class VisAreaConverter;
    class EditorVisAreaComponent;

    class EditorVisAreaConfiguration
        : public VisAreaConfiguration
    {
    public:
        AZ_TYPE_INFO(EditorVisAreaConfiguration, "{C329E65C-1F34-5C80-9A7A-4B568105256B}", VisAreaConfiguration);
        AZ_CLASS_ALLOCATOR(EditorVisAreaConfiguration, AZ::SystemAllocator,0);

        static void Reflect(AZ::ReflectContext* context);

        void ChangeHeight() override;
        void ChangeDisplayFilled() override;
        void ChangeAffectedBySun() override;
        void ChangeViewDistRatio() override;
        void ChangeOceanIsVisible() override;
        void ChangeVertexContainer() override;

        EditorVisAreaComponent* m_component = nullptr;
    };

    class EditorVisAreaComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private VisAreaComponentRequestBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AzToolsFramework::EditorEntityInfoNotificationBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
        friend class VisAreaConverter;
        friend class EditorVisAreaConfiguration; //So that the config can set m_vertices when the vertex container changes

        using Base = AzToolsFramework::Components::EditorComponentBase;

    public:
        AZ_COMPONENT(EditorVisAreaComponent, "{F4EC32D8-D4DD-54F7-97A8-D195497D5F2C}", AzToolsFramework::Components::EditorComponentBase);
        
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);

        EditorVisAreaComponent();
        virtual ~EditorVisAreaComponent();

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        /// Apply the component's settings to the underlying vis area
        void UpdateVisArea();

        void SetHeight(const float value) override
        {
            m_config.m_Height = value;
            UpdateVisArea();
        }
        float GetHeight() override
        {
            return m_config.m_Height;
        }

        void SetDisplayFilled(const bool value) override
        {
            m_config.m_DisplayFilled = value;
            UpdateVisArea();
        }
        bool GetDisplayFilled() override
        {
            return m_config.m_DisplayFilled;
        }

        void SetAffectedBySun(const bool value) override
        {
            m_config.m_AffectedBySun = value;
            UpdateVisArea();
        }
        bool GetAffectedBySun() override
        {
            return m_config.m_AffectedBySun;
        }

        void SetViewDistRatio(const float value) override
        {
            m_config.m_ViewDistRatio = value;
            UpdateVisArea();
        }
        float GetViewDistRatio() override
        {
            return m_config.m_ViewDistRatio;
        }

        void SetOceanIsVisible(const bool value) override
        {
            m_config.m_OceanIsVisible = value;
            UpdateVisArea();
        }
        bool GetOceanIsVisible() override
        {
            return m_config.m_OceanIsVisible;
        }
        
        void SetVertices(const AZStd::vector<AZ::Vector3>& value) override
        {
            m_config.m_vertexContainer.SetVertices(value);
            UpdateVisArea();
        }
        const AZStd::vector<AZ::Vector3>& GetVertices() override
        {
            return m_config.m_vertexContainer.GetVertices();
        }

        void DisplayEntity(bool& handled) override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
		AZ_DISABLE_COPY_MOVE(EditorVisAreaComponent)

        // EntitySelectionEventsBus
        void OnSelected() override;
        void OnDeselected() override;

        // Manipulator handling
        void CreateManipulators();

        // Reflected members
        EditorVisAreaConfiguration m_config; ///< Reflected configuration

        // Statics
        static AZ::Color s_visAreaColor; ///< The orange color that all visareas draw with

        // Unreflected members
        IVisArea* m_area;

        AzToolsFramework::EditorVertexSelectionVariable<AZ::Vector3> m_vertexSelection; ///< Handles all manipulator interactions with vertices (inserting and translating).
        
        AZ::Transform m_currentWorldTransform;
    };

    class VisAreaConverter 
        : public AZ::LegacyConversion::LegacyConversionEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(VisAreaConverter, AZ::SystemAllocator, 0);

        VisAreaConverter() {};
        virtual ~VisAreaConverter() {};

        // ------------------- LegacyConversionEventBus::Handler -----------------------------
        AZ::LegacyConversion::LegacyConversionResult ConvertEntity(CBaseObject* pEntityToConvert) override;
        bool BeforeConversionBegins() override;
        bool AfterConversionEnds() override;
        // END ----------------LegacyConversionEventBus::Handler ------------------------------
    };
   
} // namespace Visibility
