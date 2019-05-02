/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
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

#include <LmbrCentral/Animation/CharacterAnimationBus.h>

namespace Footsteps
{
    class FootstepComponent
        : public AZ::Component
        , protected LmbrCentral::CharacterAnimationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(FootstepComponent, "{C5635496-0A46-4D2C-8CE3-D7F642E54FC5}");

        static void Reflect(AZ::ReflectContext* context);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CharacterAnimationNotificationBus interface implementation
        void OnAnimationEvent(const LmbrCentral::AnimationEvent& event) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
