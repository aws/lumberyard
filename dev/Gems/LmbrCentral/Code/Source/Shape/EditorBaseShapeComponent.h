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

#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Shape/EditorShapeComponentBus.h>

namespace LmbrCentral
{
    class EditorBaseShapeComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public LmbrCentral::EditorShapeComponentRequestsBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:

        AZ_RTTI(EditorBaseShapeComponent, "{32B9D7E9-6743-427B-BAFD-1C42CFBE4879}", AzToolsFramework::Components::EditorComponentBase);
        EditorBaseShapeComponent();
        ~EditorBaseShapeComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        void DisplayEntity(bool& handled) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////
        // Transform notification bus handler
        /// Called when the local transform of the entity has changed.
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;
        //////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////
        // LmbrCentral::EditorShapeComponentRequestsBus
        void SetShapeColor(const AZ::Vector4& solidColor) override;
        void SetShapeWireframeColor(const AZ::Vector4& wireColor) override;
        //////////////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        virtual void DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const {}

        //! Stores the transform of the entity
        AZ::Transform m_currentEntityTransform;

        // Colors used for debug visualizations
        AZ::Vector4 m_shapeColor;
        AZ::Vector4 m_shapeWireColor;

        // Colors used for debug visualizations
        static const AZ::Vector4 s_shapeColor;
        static const AZ::Vector4 s_shapeWireColor;
    };
} // namespace LmbrCentral
