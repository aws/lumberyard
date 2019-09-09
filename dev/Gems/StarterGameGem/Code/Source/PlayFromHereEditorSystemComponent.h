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
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>


namespace StarterGameGem
{
    /*!
    * PlayFromHereEditorSystemComponentRequests
    * Messages serviced by the PlayFromHereEditorSystemComponent.
    */
    class PlayFromHereEditorSystemComponentRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~PlayFromHereEditorSystemComponentRequests() = default;

        virtual void PlayFromHere() {}
    };
    using PlayFromHereEditorSystemComponentRequestBus = AZ::EBus<PlayFromHereEditorSystemComponentRequests>;


    class PlayFromHereEditorSystemComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides (Configuring this Ebus)
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        ////////////////////////////////////////////////////////////////////////

        /**
        * Notifies all 'Play From Here' entities to move to the specified location.
        */
        virtual void OnPlayFromHere(const AZ::Vector3& pos) {}
    };
    using PlayFromHereEditorSystemComponentNotificationBus = AZ::EBus<PlayFromHereEditorSystemComponentNotifications>;


    class PlayFromHereEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private PlayFromHereEditorSystemComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PlayFromHereEditorSystemComponent, "{42DBAD81-E891-4336-961C-35386335ACC6}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PlayFromHereSystemService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("PlayFromHereSystemService"));
        }

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEvents
        void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEntityContextNotificationBus implementation
        void OnStartPlayInEditor() override;
        void OnStopPlayInEditor() override {}
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // PlayFromHereEditorSystemComponentRequestBus interface implementation
        void PlayFromHere() override;
        ////////////////////////////////////////////////////////////////////////

        bool m_errorFindingLocation;
        AZ::Vector2 m_contextMenuViewPoint;
        AZ::Vector3 m_playFromHere;
    };
}
