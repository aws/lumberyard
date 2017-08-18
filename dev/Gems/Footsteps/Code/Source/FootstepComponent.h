
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
