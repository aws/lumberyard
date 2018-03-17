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
#include "LmbrCentral_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <LmbrCentral/Animation/CharacterAnimationBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include "MotionParameterSmoothingComponent.h"

namespace LmbrCentral
{
    void MotionParameterSmoothingComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            using namespace Animation;

            serializeContext->Class<MotionParameterSmoothingSettings>()
                ->Version(1)
                ->Field("GroundAngleConvergeTime", &MotionParameterSmoothingSettings::m_groundAngleConvergeTime)
                ->Field("TravelAngleConvergeTime", &MotionParameterSmoothingSettings::m_travelAngleConvergeTime)
                ->Field("TravelDistanceConvergeTime", &MotionParameterSmoothingSettings::m_travelDistanceConvergeTime)
                ->Field("TravelSpeedConvergeTime", &MotionParameterSmoothingSettings::m_travelSpeedConvergeTime)
                ->Field("TurnAngleConvergeTime", &MotionParameterSmoothingSettings::m_turnAngleConvergeTime)
                ->Field("TurnSpeedConvergeTime", &MotionParameterSmoothingSettings::m_turnSpeedConvergeTime)
                ;

            serializeContext->Class<MotionParameterSmoothingComponent, AZ::Component>()
                ->Version(1)
                ->Field("Settings", &MotionParameterSmoothingComponent::m_settings)
                ;

#ifdef ENABLE_LEGACY_ANIMATION
            AZ::EditContext* edit = serializeContext->GetEditContext();
            if (edit)
            {
                edit->Class<MotionParameterSmoothingSettings>(
                    "Smoothing Settings", "Motion parameter smoothing controls")
                    ->DataElement(0, &MotionParameterSmoothingSettings::m_groundAngleConvergeTime, "Ground angle time", "Time (in seconds) to converge toward actual ground angle")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(0, &MotionParameterSmoothingSettings::m_travelAngleConvergeTime, "Travel angle time", "Time (in seconds) to converge toward actual travel angle")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(0, &MotionParameterSmoothingSettings::m_travelDistanceConvergeTime, "Travel distance time", "Time (in seconds) to converge toward actual travel distance")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(0, &MotionParameterSmoothingSettings::m_travelSpeedConvergeTime, "Travel speed time", "Time (in seconds) to converge toward actual travel speed")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(0, &MotionParameterSmoothingSettings::m_turnAngleConvergeTime, "Turn angle time", "Time (in seconds) to converge toward actual turn angle")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(0, &MotionParameterSmoothingSettings::m_turnSpeedConvergeTime, "Turn speed time", "Time (in seconds) to converge toward actual turn speed")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ;
                
                edit->Class<MotionParameterSmoothingComponent>(
                    "Motion Parameter Smoothing", "The Motion Parameter Smoothing component allows configuration of the animation blend parameter behavior (for blend spaces) for a specified character instance")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Animation (Legacy)")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/MotionParameterSmoothing.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/MotionParameterSmoothing.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-motion-parameter-smoothing.html")
                    ->DataElement(0, &MotionParameterSmoothingComponent::m_settings, "Smoothing settings", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                    ;
            }
#endif
        }
    }

    void MotionParameterSmoothingComponent::Activate()
    {
        // If the character instance is already ready, apply settings immediately, otherwise listen for when it's ready.
        ICharacterInstance* character = nullptr;
        SkinnedMeshComponentRequestBus::EventResult(character, GetEntityId(), &SkinnedMeshComponentRequests::GetCharacterInstance);
        if (character)
        {
            OnCharacterInstanceRegistered(character);
        }

        CharacterAnimationNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void MotionParameterSmoothingComponent::Deactivate()
    {
        CharacterAnimationNotificationBus::Handler::BusDisconnect(GetEntityId());
    }

    void MotionParameterSmoothingComponent::OnCharacterInstanceRegistered(ICharacterInstance* character)
    {
        (void)character;
        CharacterAnimationRequestBus::Event(GetEntityId(), &CharacterAnimationRequests::SetMotionParameterSmoothingSettings, m_settings);
    }

    //////////////////////////////////////////////////////////////////////////
} // namespace LmbrCentral
