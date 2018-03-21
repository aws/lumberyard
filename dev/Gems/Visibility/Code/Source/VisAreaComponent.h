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

#include <AzCore/EBus/EBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

#include <AzCore/Math/VertexContainer.h>
#include <AzCore/Math/Vector3.h>

#include <VisAreaComponentBus.h>

namespace Visibility
{
    class VisAreaConfiguration
    {
    public:
        AZ_TYPE_INFO(VisAreaConfiguration, "{160D9FC2-936F-59BB-827C-DEF89671E4DC}");
        AZ_CLASS_ALLOCATOR(VisAreaConfiguration, AZ::SystemAllocator,0);

        virtual ~VisAreaConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        float m_Height = 5.0f;
        bool m_DisplayFilled = false;
        bool m_AffectedBySun = false;
        float m_ViewDistRatio = 100.0f;
        bool m_OceanIsVisible = false;
        AZ::VertexContainer<AZ::Vector3> m_vertexContainer;

        virtual void ChangeHeight() {}
        virtual void ChangeDisplayFilled() {}
        virtual void ChangeAffectedBySun() {}
        virtual void ChangeViewDistRatio() {}
        virtual void ChangeOceanIsVisible() {}
        virtual void ChangeVertexContainer() {}
    };

    class VisAreaComponent
        : public AZ::Component
        , private VisAreaComponentRequestBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(VisAreaComponent, "{ACAB60F8-100E-5EAF-BE2B-D60F79312404}", AZ::Component);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);

        VisAreaComponent() = default;
        explicit VisAreaComponent(VisAreaConfiguration *params) {
            m_config = *params;
        }
        
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // VisAreaComponentRequestBus
        void SetHeight(const float value) override;
        float GetHeight() override;
        void SetDisplayFilled(const bool value) override;
        bool GetDisplayFilled() override;
        void SetAffectedBySun(const bool value) override;
        bool GetAffectedBySun() override;
        void SetViewDistRatio(const float value) override;
        float GetViewDistRatio() override;
        void SetOceanIsVisible(const bool value) override;
        bool GetOceanIsVisible() override;
        void SetVertices(const AZStd::vector<AZ::Vector3>& value) override;
        const AZStd::vector<AZ::Vector3>& GetVertices() override;

    protected:
        VisAreaConfiguration m_config; ///< Reflected configuration
    };
} // namespace Visibility
