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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include "ConstraintComponent.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace LmbrCentral
{
    /**
    * Extends EditorConstraintConfiguration structure to add editor functionality
    * such as property handlers and visibility filters, as well as
    * reflection for editing.
    */
    class EditorConstraintConfiguration : public ConstraintConfiguration
    {
    public:
        AZ_TYPE_INFO(EditorConstraintConfiguration, "{43ECDE4D-BCFE-4F5A-9F15-70EAA9F9A757}", ConstraintConfiguration);

        void Init(const AZ::EntityId& self);

        //////////////////////////////////////////////////////////////////////////
        // Visibility callbacks
        bool GetOwningEntityVisibility() const override;
        bool GetTargetEntityVisibility() const override;

        bool GetAxisVisibility() const override;
        bool GetEnableRotationVisibility() const override;
        bool GetPartIdVisibility() const override;
        bool GetForceLimitVisibility() const override;

        bool GetRotationLimitGroupVisibility() const override;
        bool GetRotationLimitVisibilityX() const override;
        bool GetRotationLimitVisibilityYZ() const override;

        bool GetSearchRadiusGroupVisibility() const override;
        bool GetSearchRadiusVisibility() const override;

        bool GetDampingVisibility() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // OnChange handlers
        AZ::Crc32 OnConstraintTypeChanged() override;
        AZ::Crc32 OnOwnerTypeChanged() override;
        AZ::Crc32 OnTargetTypeChanged() override;
        AZ::Crc32 OnPropertyChanged() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Other callbacks
        const char * GetXLimitUnits() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Static methods
        static void Reflect(AZ::ReflectContext* context);
        //////////////////////////////////////////////////////////////////////////

    private:
        // Non-serialized members
        AZ::EntityId m_self;
    };

    /*!
    * In-editor constraint component.
    * Handles creating and editing physics constraints in the editor.
    */
    class EditorConstraintComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    private:
        using Base = AzToolsFramework::Components::EditorComponentBase;
    public:
        AZ_COMPONENT(EditorConstraintComponent, "{998FB21C-9796-4032-8391-623B6CAAE54E}", AzToolsFramework::Components::EditorComponentBase);

        EditorConstraintComponent() {}
        ~EditorConstraintComponent() override {}

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //! AzToolsFramework::Components::EditorComponentBase implementation
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::EntityDebugDisplayEventBus interface implementation
        void DisplayEntity(bool& handled) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // static methods
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            ConstraintComponent::GetProvidedServices(provided);
        }
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            ConstraintComponent::GetDependentServices(dependent);
        }

        static void Reflect(AZ::ReflectContext* context);
        //////////////////////////////////////////////////////////////////////////

    private:
        EditorConstraintConfiguration m_config;
    };
} // namespace LmbrCentral
