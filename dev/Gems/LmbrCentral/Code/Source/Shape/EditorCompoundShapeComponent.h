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
#include <LmbrCentral/Shape/CompoundShapeComponentBus.h>

namespace LmbrCentral
{
    class EditorCompoundShapeComponent
        : public EditorBaseShapeComponent
    {
    public:

        AZ_EDITOR_COMPONENT(EditorCompoundShapeComponent, "{837AA0DF-9C14-4311-8410-E7983E1F4B8D}", EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        ~EditorCompoundShapeComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase implementation
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("CompoundShapeService", 0x4f7c640a));
        }

    private:

        //! Stores configuration for this component
        CompoundShapeConfiguration m_configuration;
    };
} // namespace LmbrCentral
