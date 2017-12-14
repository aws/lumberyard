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

#include "MannequinComponent.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <LmbrCentral/Animation/MannequinComponentBus.h>

namespace LmbrCentral
{
    class EditorMannequinComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private MannequinInformationBus::Handler
    {
    public:

        AZ_COMPONENT(EditorMannequinComponent, "{C5E08FE6-E1FC-4080-A053-2C65A667FE82}",
            AzToolsFramework::Components::EditorComponentBase);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MannequinInformationBus Implementation
        void FetchAvailableScopeContextNames(AZStd::vector<AZStd::string>& scopeContextNames) const override;
        //////////////////////////////////////////////////////////////////////////

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AnimationService", 0x553f5760));
            provided.push_back(AZ_CRC("MannequinService", 0x424b0eea));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AnimationService", 0x553f5760));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:

        //////////////////////////////////////////////////////////////////////////
        // Helpers
        void UpdateScopeContextNames();
        void ControllerDefinitionChanged();
        //////////////////////////////////////////////////////////////////////////

        // Reference to the animation controller definition file asset
        AzFramework::SimpleAssetReference<MannequinControllerDefinitionAsset> m_controllerDefinition;

        // Stores a list of all available scope context names for this controller definition
        AZStd::vector<AZStd::string> m_scopeContextNames;
    };
} // namespace LmbrCentral
