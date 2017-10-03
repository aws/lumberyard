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

#include "EditorBaseShapeComponent.h"
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include "CylinderShape.h"

namespace LmbrCentral
{
    class EditorCylinderShapeComponent
        : public EditorBaseShapeComponent
        , public CylinderShape
    {
    public:

        AZ_EDITOR_COMPONENT(EditorCylinderShapeComponent, EditorCylinderShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        ~EditorCylinderShapeComponent() override = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase implementation
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        void DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const override;
        ////////////////////////////////////////////////////////////////////////

        // CylinderShape
        CylinderShapeConfig& GetConfiguration() override { return m_configuration; }

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("CylinderShapeService", 0x507c688e));
        }

    private:
        void ConfigurationChanged();

        //! Stores configuration of a cylinder for this component
        CylinderShapeConfig m_configuration;
    };
} // namespace LmbrCentral
