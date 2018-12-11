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

#ifdef STARTER_GAME_EDITOR
#include "PlayFromHereEditorSystemComponent.h"
#endif

#include <AzCore/Component/Component.h>


namespace StarterGameGem
{
    class PlayFromHereComponent
        : public AZ::Component
#ifdef STARTER_GAME_EDITOR
        , private PlayFromHereEditorSystemComponentNotificationBus::Handler
#endif
    {
    public:
        AZ_COMPONENT(PlayFromHereComponent, "{B2192B24-A38C-449A-82E6-248CDD885771}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PlayFromHereService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("PlayFromHereService"));
        }

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
#ifdef STARTER_GAME_EDITOR
        ////////////////////////////////////////////////////////////////////////
        // StarterGameEditorSystemComponentNotificationBus::Handler overrides
        void OnPlayFromHere(const AZ::Vector3& pos) override;
        ////////////////////////////////////////////////////////////////////////
#endif
    };
}
