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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>

#include <AzFramework/Asset/SimpleAsset.h>

#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>

namespace LightningArc
{
    // Request bus for the component
    class LightningBus
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBus Traits overrides (Configuring this Ebus)
        // Using Defaults
        //////////////////////////////////////////////////////////////////////////

        virtual ~LightningBus() {}

        /////////////////////////////////////////////////////////////////
        // Property Set/Get functions for the component.
        /////////////////////////////////////////////////////////////////
        virtual void SetOutdoorOnly(const bool value) = 0;
        virtual bool GetOutdoorOnly() = 0;
        virtual void SetCastShadow(const bool value) = 0;
        virtual bool GetCastShadow() = 0;
        virtual void SetCastShadowMinspec(const int value) = 0;
        virtual int GetCastShadowMinspec() = 0;
        virtual void SetLodRatio(const int value) = 0;
        virtual int GetLodRatio() = 0;
        virtual void SetViewDistanceMultiplier(const float value) = 0;
        virtual float GetViewDistanceMultiplier() = 0;
        virtual void SetHiddenInGame(const bool value) = 0;
        virtual bool GetHiddenInGame() = 0;
        virtual void SetRecvWind(const bool value) = 0;
        virtual bool GetRecvWind() = 0;
        virtual void SetRenderNearest(const bool value) = 0;
        virtual bool GetRenderNearest() = 0;
        virtual void SetNoStaticDecals(const bool value) = 0;
        virtual bool GetNoStaticDecals() = 0;
        virtual void SetCreatedThroughPool(const bool value) = 0;
        virtual bool GetCreatedThroughPool() = 0;
        virtual void SetObstructionMultiplier(const float value) = 0;
        virtual float GetObstructionMultiplier() = 0;
        virtual void SetbActive(const bool value) = 0;
        virtual bool GetbActive() = 0;
        virtual void SetbRelativeToPlayer(const bool value) = 0;
        virtual bool GetbRelativeToPlayer() = 0;
        virtual void SetfDistance(const float value) = 0;
        virtual float GetfDistance() = 0;
        virtual void SetAudio_audioTriggerStopTrigger(const AZStd::string& value) = 0;
        virtual AZStd::string GetAudio_audioTriggerStopTrigger() = 0;
        virtual void SetAudio_audioRTPCDistanceRtpc(const AZStd::string& value) = 0;
        virtual AZStd::string GetAudio_audioRTPCDistanceRtpc() = 0;
        virtual void SetAudio_fSpeedOfSoundScale(const float value) = 0;
        virtual float GetAudio_fSpeedOfSoundScale() = 0;
        virtual void SetEffects_color_SkyHighlightColor(const AZ::Vector3& value) = 0;
        virtual AZ::Vector3 GetEffects_color_SkyHighlightColor() = 0;
        virtual void SetEffects_SkyHighlightMultiplier(const float value) = 0;
        virtual float GetEffects_SkyHighlightMultiplier() = 0;
        virtual void SetEffects_ParticleEffect(const AZStd::string& value) = 0;
        virtual AZStd::string GetEffects_ParticleEffect() = 0;
        virtual void SetEffects_LightRadius(const float value) = 0;
        virtual float GetEffects_LightRadius() = 0;
        virtual void SetEffects_LightIntensity(const float value) = 0;
        virtual float GetEffects_LightIntensity() = 0;
        virtual void SetEffects_ParticleScale(const float value) = 0;
        virtual float GetEffects_ParticleScale() = 0;
        virtual void SetEffects_SkyHighlightVerticalOffset(const float value) = 0;
        virtual float GetEffects_SkyHighlightVerticalOffset() = 0;
        virtual void SetEffects_SkyHighlightAtten(const float value) = 0;
        virtual float GetEffects_SkyHighlightAtten() = 0;
        virtual void SetTiming_fDelay(const float value) = 0;
        virtual float GetTiming_fDelay() = 0;
        virtual void SetTiming_fDelayVariation(const float value) = 0;
        virtual float GetTiming_fDelayVariation() = 0;
    };

    using LightningRequestBus = AZ::EBus<LightningBus>;

} // namespace LightningArc
