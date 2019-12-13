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
#include <AzFramework/Physics/Shape.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <PhysX/ConfigurationBus.h>
#include <Editor/DebugDraw.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace PhysX
{
    /// Editor PhysX Shape Collider Component.
    /// This component is used together with a shape component, and uses the shape information contained in that
    /// component to create geometry in the PhysX simulation.
    class EditorShapeColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzFramework::EntityDebugDisplayEventBus::Handler
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private PhysX::ConfigurationNotificationBus::Handler
        , protected DebugDraw::DisplayCallback
        , protected LmbrCentral::ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorShapeColliderComponent, "{2389DDC7-871B-42C6-9C95-2A679DDA0158}",
            AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        EditorShapeColliderComponent();

        // this function is made virtual because we call it from other modules
        virtual const Physics::ColliderConfiguration& GetColliderConfiguration() const;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        void CreateStaticEditorCollider();
        AZ::u32 OnConfigurationChanged();
        void CheckSupportedShapeTypes();

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EntitySelectionEvents
        void OnSelected() override;
        void OnDeselected() override;

        // TransformBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // PhysX::ConfigurationNotificationBus
        void OnConfigurationRefreshed(const Configuration& configuration) override;
        void OnDefaultMaterialLibraryChanged(const AZ::Data::AssetId& defaultMaterialLibrary) override;

        // LmbrCentral::ShapeComponentNotificationBus
        void OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons changeReason) override;

        // DisplayCallback
        void Display(AzFramework::DebugDisplayRequests& debugDisplay) const;

        Physics::ColliderConfiguration m_colliderConfig;
        DebugDraw::Collider m_colliderDebugDraw;
        AZ::Vector3 m_scale = AZ::Vector3::CreateOne();
        AZStd::unique_ptr<Physics::RigidBodyStatic> m_editorBody;
        bool m_shapeTypeWarningIssued = false;
    };
} // namespace PhysX
