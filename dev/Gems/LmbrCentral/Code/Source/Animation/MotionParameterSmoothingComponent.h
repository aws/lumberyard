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

#include <LmbrCentral/Animation/MotionExtraction.h>
#include <LmbrCentral/Animation/CharacterAnimationBus.h>

namespace LmbrCentral
{
    class MotionParameterSmoothingComponent
        : public AZ::Component
        , private CharacterAnimationNotificationBus::Handler
    {
    public:

        AZ_COMPONENT(MotionParameterSmoothingComponent, "{C927CF87-CD02-4201-BFAD-CB5956586467}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // CharacterAnimationNotifications interface implementation
        void OnCharacterInstanceRegistered(ICharacterInstance* character);
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);

    private:

        Animation::MotionParameterSmoothingSettings m_settings;
        
    };
} // namespace LmbrCentral
