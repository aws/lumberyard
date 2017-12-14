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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <LmbrCentral/Animation/MannequinAsset.h>
#include <LmbrCentral/Animation/MannequinComponentBus.h>


namespace LmbrCentral
{
    class EditorMannequinScopeComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private MannequinAssetNotificationBus::Handler
    {
    public:

        AZ_COMPONENT(EditorMannequinScopeComponent, "{A5E58207-3272-4848-B1A5-F739450D9D21}",
            AzToolsFramework::Components::EditorComponentBase);

        EditorMannequinScopeComponent();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MannequinInformationNotificationBus::Handler implementation
        void OnControllerDefinitionsChanged() override;
        //////////////////////////////////////////////////////////////////////////

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MannequinScopeService", 0x7adf3115));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("AnimationService", 0x553f5760));
            required.push_back(AZ_CRC("MannequinService", 0x424b0eea));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:

        //////////////////////////////////////////////////////////////////////////
        // Helpers
        AZStd::vector<AZStd::string> GetAvailableScopeContextNames();
        void RefreshAvailableScopeContextNames();
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Serialized Data
        // Reference to the animation database file
        AzFramework::SimpleAssetReference<MannequinAnimationDatabaseAsset> m_animationDatabase;

        // Entity Id for the target of this scope context setting
        AZ::EntityId m_targetEntityId;

        // Name of the scope that this anim database is to be attached to
        AZStd::string m_scopeContextName;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Runtime data
        // Stores a list of all available scope context names for this controller definition
        AZStd::vector<AZStd::string> m_scopeContextNames;
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace LmbrCentral
