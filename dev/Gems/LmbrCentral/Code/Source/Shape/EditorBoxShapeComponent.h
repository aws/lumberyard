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
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>

namespace LmbrCentral
{
    class EditorBoxShapeComponent
        : public EditorBaseShapeComponent
    {
    public:

        AZ_EDITOR_COMPONENT(EditorBoxShapeComponent, "{2ADD9043-48E8-4263-859A-72E0024372BF}", EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        ~EditorBoxShapeComponent() override = default;

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
            provided.push_back(AZ_CRC("BoxShapeService"));
        }

    private:

        //! Stores configuration of a Box for this component
        BoxShapeConfiguration m_configuration;
    };
} // namespace LmbrCentral
