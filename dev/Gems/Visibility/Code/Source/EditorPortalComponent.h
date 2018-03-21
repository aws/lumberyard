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
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <LegacyEntityConversion/LegacyEntityConversionBus.h>

#include <PortalComponentBus.h>
#include "PortalComponent.h"

namespace Visibility 
{    
    class PortalConverter;
    class EditorPortalComponent;

    class EditorPortalConfiguration 
        : public PortalConfiguration
    {
    public:
        friend class EditorPortalComponent; //So it can access m_component

        AZ_TYPE_INFO(EditorPortalConfiguration, "{C9F99449-7A77-50C4-9ED3-D69B923BFDBD}", PortalConfiguration);
        AZ_CLASS_ALLOCATOR(EditorPortalConfiguration, AZ::SystemAllocator,0);

        static void Reflect(AZ::ReflectContext* context);
        
        void OnChange() override;
        void OnVerticesChange() override;

    private:
        EditorPortalComponent* m_component = nullptr;
    };

    class EditorPortalComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private PortalRequestBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
        friend class PortalConverter;
        friend class EditorPortalConfiguration;

        using Base = AzToolsFramework::Components::EditorComponentBase;

    public:

        AZ_COMPONENT(EditorPortalComponent, "{64525CDD-7DD4-5CEF-B545-559127DC834E}", AzToolsFramework::Components::EditorComponentBase);
        
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EditorPortalService", 0x6ead38f6));
            provided.push_back(AZ_CRC("PortalService", 0x06076210));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("QuadShapeService", 0xe449b0fc));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SphereShapeService", 0x90c8dc80));
            incompatible.push_back(AZ_CRC("SplineShapeService", 0x4d4b94a2));
            incompatible.push_back(AZ_CRC("PolygonPrismShapeService", 0x1cbc4ed4));
        }

        static void Reflect(AZ::ReflectContext* context);

        IVisArea* m_area = nullptr;

        EditorPortalComponent();
        ~EditorPortalComponent() override;

        // AzToolsFramework::Components::EditorComponentBase
        void Activate() override;
        void Deactivate() override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        /// Update the object runtime after changes to the Configuration.
        /// Called by the default RequestBus SetXXX implementations,
        /// and used to initially set up the object the first time the
        /// Configuration are set.
        void UpdateObject();

        void SetHeight(const float value) override
        {
            m_config.m_height = value;
            UpdateObject();
        }
        float GetHeight() override
        {
            return m_config.m_height;
        }

        void SetDisplayFilled(const bool value) override
        {
            m_config.m_displayFilled = value;
            UpdateObject();
        }
        bool GetDisplayFilled() override
        {
            return m_config.m_displayFilled;
        }

        void SetAffectedBySun(const bool value) override
        {
            m_config.m_affectedBySun = value;
            UpdateObject();
        }
        bool GetAffectedBySun() override
        {
            return m_config.m_affectedBySun;
        }

        void SetViewDistRatio(const float value) override
        {
            m_config.m_viewDistRatio = value;
            UpdateObject();
        }
        float GetViewDistRatio() override
        {
            return m_config.m_viewDistRatio;
        }

        void SetSkyOnly(const bool value) override
        {
            m_config.m_skyOnly = value;
            UpdateObject();
        }
        bool GetSkyOnly() override
        {
            return m_config.m_skyOnly;
        }

        void SetOceanIsVisible(const bool value) override
        {
            m_config.m_oceanIsVisible = value;
            UpdateObject();
        }
        bool GetOceanIsVisible() override
        {
            return m_config.m_oceanIsVisible;
        }

        void SetUseDeepness(const bool value) override
        {
            m_config.m_useDeepness = value;
            UpdateObject();
        }
        bool GetUseDeepness() override
        {
            return m_config.m_useDeepness;
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

        void SetLightBlending(const bool value) override
        {
            m_config.m_lightBlending = value;
            UpdateObject();
        }
        bool GetLightBlending() override
        {
            return m_config.m_lightBlending;
        }

        void SetLightBlendValue(const float value) override
        {
            m_config.m_lightBlendValue = value;
            UpdateObject();
        }
        float GetLightBlendValue() override
        {
            return m_config.m_lightBlendValue;
        }
        
        // EntityDebugDisplayBus
        void DisplayEntity(bool& handled) override;

    private:
		AZ_DISABLE_COPY_MOVE(EditorPortalComponent)

        // EntitySelectionEventsBus
        void OnSelected() override;
        void OnDeselected() override;

        // Manipulator handling
        void CreateManipulators();

        //Reflected members
        EditorPortalConfiguration m_config;

        AzToolsFramework::EditorVertexSelectionFixed<AZ::Vector3> m_vertexSelection; ///< Handles all manipulator interactions with vertices.

        AZ::Transform m_AZCachedWorldTransform;
        Matrix44 m_cryCachedWorldTransform;
    };

    class PortalConverter : public AZ::LegacyConversion::LegacyConversionEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PortalConverter, AZ::SystemAllocator, 0);

        PortalConverter() {}

        // ------------------- LegacyConversionEventBus::Handler -----------------------------
        AZ::LegacyConversion::LegacyConversionResult ConvertEntity(CBaseObject* pEntityToConvert) override;
        bool BeforeConversionBegins() override;
        bool AfterConversionEnds() override;
        // END ----------------LegacyConversionEventBus::Handler ------------------------------
    };
   
} // namespace Visibility
