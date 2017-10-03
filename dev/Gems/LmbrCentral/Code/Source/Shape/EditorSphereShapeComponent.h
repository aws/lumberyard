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
#include "SphereShapeComponent.h"
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>

namespace LmbrCentral
{   
    class EditorSphereShapeComponent
        : public EditorBaseShapeComponent
        , public SphereShape
    {
    public:

        AZ_EDITOR_COMPONENT(EditorSphereShapeComponent, EditorSphereShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        ~EditorSphereShapeComponent() override = default;
        
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;        
        
        // EditorComponentBase implementation
        void BuildGameEntity(AZ::Entity* gameEntity) override;                
        void DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const override;        

        SphereShapeConfig& GetConfiguration() override
        {
            return m_configuration;
        }
    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("SphereShapeService", 0x90c8dc80));
        }

    private:

        void ConfigurationChanged();

        SphereShapeConfig m_configuration;
    };
} // namespace LmbrCentral
