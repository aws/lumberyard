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
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>

namespace LmbrCentral
{
    class EditorSphereShapeComponent
        : public EditorBaseShapeComponent
    {
    public:

        AZ_EDITOR_COMPONENT(EditorSphereShapeComponent, "{2EA56CBF-63C8-41D9-84D5-0EC2BECE748E}", EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        ~EditorSphereShapeComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase implementation
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        void DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const override;
        ////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("SphereShapeService"));
        }

    private:

        //! Stores configuration of a sphere for this component
        SphereShapeConfiguration m_configuration;
    };
} // namespace LmbrCentral
