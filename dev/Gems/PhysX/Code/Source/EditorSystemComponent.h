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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/SystemBus.h>

namespace PhysX
{
    ///
    /// System component for PhysX editor
    ///
    class EditorSystemComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public Physics::EditorWorldBus::Handler
    {
    public:
        AZ_COMPONENT(EditorSystemComponent, "{560F08DC-94F5-4D29-9AD4-CDFB3B57C654}");
        static void Reflect(AZ::ReflectContext* context);

        EditorSystemComponent() = default;

        // Physics::EditorWorldBus
        AZStd::shared_ptr<Physics::World> GetEditorWorld() override;
        void MarkEditorWorldDirty() override;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysXEditorService", 0x0a61cda5));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("PhysXService", 0x75beae2d));
        }

        // TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        AZStd::shared_ptr<Physics::World> m_editorWorld;
        bool m_editorWorldDirty = false; ///< When true, it's time to update the editor world.
        static const float s_minEditorWorldUpdateInterval; ///< The minimal interval between editor World updates.
        static const float s_fixedDeltaTime; ///< The fixed delta time used to update the editor world.
        float m_intervalCountdown = s_minEditorWorldUpdateInterval;
    };
}
