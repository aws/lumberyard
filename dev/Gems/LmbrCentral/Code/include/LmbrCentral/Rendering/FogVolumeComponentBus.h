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

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    class FogVolumeConfiguration;

    enum class FogVolumeType : AZ::s32
    {
        None = -1,
        Ellipsoid = 0,
        RectangularPrism = 1,
    };

    class FogVolumeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~FogVolumeComponentRequests() {}

        /// Propagates updated configuration to the fog volume component
        virtual void RefreshFog() {};

        virtual FogVolumeType GetVolumeType() { return FogVolumeType::None; }
        virtual void SetVolumeType(FogVolumeType volumeType) {};

        virtual AZ::Color GetColor() { return AZ::Color::CreateOne(); }
        virtual void SetColor(AZ::Color color) {};

        virtual float GetHdrDynamic() { return FLT_MAX; }
        virtual void SetHdrDynamic(float hdrDynamic) {};

        virtual bool GetUseGlobalFogColor() { return false; }
        virtual void SetUseGlobalFogColor(bool useGlobalFogColor) {};

        virtual float GetGlobalDensity() { return FLT_MAX; }
        virtual void SetGlobalDensity(float globalDensity) {};

        virtual float GetDensityOffset() { return FLT_MAX; }
        virtual void SetDensityOffset(float densityOffset) {};

        virtual float GetNearCutoff() { return FLT_MAX; }
        virtual void SetNearCutoff(float nearCutoff) {};

        virtual float GetFallOffDirLong() { return FLT_MAX; }
        virtual void SetFallOffDirLong(float fallOffDirLong) {};

        virtual float GetFallOffDirLatitude() { return FLT_MAX; }
        virtual void SetFallOffDirLatitude(float fallOffDirLatitude) {};

        virtual float GetFallOffShift() { return FLT_MAX; }
        virtual void SetFallOffShift(float fallOffShift) {};

        virtual float GetFallOffScale() { return FLT_MAX; }
        virtual void SetFallOffScale(float fallOffScale) {};

        virtual float GetSoftEdges() { return FLT_MAX; }
        virtual void SetSoftEdges(float softEdges) {};

        virtual float GetRampStart() { return FLT_MAX; }
        virtual void SetRampStart(float rampStart) {};

        virtual float GetRampEnd() { return FLT_MAX; }
        virtual void SetRampEnd(float rampEnd) {};

        virtual float GetRampInfluence() { return FLT_MAX; }
        virtual void SetRampInfluence(float rampInfluence) {};

        virtual float GetWindInfluence() { return FLT_MAX; }
        virtual void SetWindInfluence(float windInfluence) {};

        virtual float GetDensityNoiseScale() { return FLT_MAX; }
        virtual void SetDensityNoiseScale(float densityNoiseScale) {};

        virtual float GetDensityNoiseOffset() { return FLT_MAX; }
        virtual void SetDensityNoiseOffset(float densityNoiseOffset) {};

        virtual float GetDensityNoiseTimeFrequency() { return FLT_MAX; }
        virtual void SetDensityNoiseTimeFrequency(float timeFrequency) {};

        virtual AZ::Vector3 GetDensityNoiseFrequency() { return AZ::Vector3::CreateOne(); }
        virtual void SetDensityNoiseFrequency(AZ::Vector3 densityNoiseFrequency) {};

        virtual bool GetIgnoresVisAreas() { return false; };
        virtual void SetIgnoresVisAreas(bool ignoresVisAreas) {};

        virtual bool GetAffectsThisAreaOnly() { return false; };
        virtual void SetAffectsThisAreaOnly(bool affectsThisAreaOnly) {};
    };

    using FogVolumeComponentRequestBus = AZ::EBus<FogVolumeComponentRequests>;
}