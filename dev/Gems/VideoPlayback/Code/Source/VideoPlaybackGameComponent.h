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

/*
    This video playback game component's job is to provide an interface for decoding and playing
    video. The decoding and playback is handled by the Decoder class which wraps FFMPEG. This class
    is responsible for taking decoded frames and sending them to Lumberyard as usable textures.
*/

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzCore/Component/TickBus.h>
#include <ITexture.h>
#include "VideoPlaybackAsset.h"
#include <Include/VideoPlayback/VideoPlaybackBus.h>
#include "Decoder.h"
#include <VideoPlayback_Traits_Platform.h>

#if AZ_TRAIT_VIDEOPLAYBACK_ENABLE_DECODER

namespace AZ 
{
    namespace VideoPlayback
    {
        class VideoPlaybackGameComponent
            : public AZ::Component
            , public VideoPlaybackRequestBus::Handler
            , public TickBus::Handler
        {
        public:
            AZ_COMPONENT(VideoPlaybackGameComponent, "{CA4F2A0B-CF7E-46FD-A7F6-A9279628164C}");
            virtual ~VideoPlaybackGameComponent() = default;

            static void Reflect(AZ::ReflectContext* reflection);

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // VideoPlaybackRequestBus
            void Play() override;
            void Pause() override;
            void Stop() override;
            void EnableLooping(bool enable) override;
            bool IsPlaying() override;
            void SetPlaybackSpeed(float speedFactor) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // OnTick bus
            void OnTick(float deltaTime, AZ::ScriptTimePoint time);
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Editor events
            AZ::Crc32 VideoPlaybackGameComponent::OnRenderTextureChange();
            //////////////////////////////////////////////////////////////////////////

        private:
            AZ::EntityId m_entityId;
            AzFramework::SimpleAssetReference<VideoPlaybackAsset> m_videoAsset;
            AZStd::string m_userTextureName;
            uint32 m_queueAheadCount = 1;
            Decoder m_videoDecoder;

            bool m_playing = false;
            bool m_shouldStop = false;
            bool m_shouldLoop = false;

            bool m_isStereo = false;
            AZ::VR::StereoLayout m_preferredStereoLayout = AZ::VR::StereoLayout::UNKNOWN;   //UNKNOWN in this case simply means "try to determine automatically"

            Decoder::FrameInfo m_frame;

            unsigned int m_videoTextureLeft = 0;
            unsigned int m_videoTextureRight = 0;

            float m_secondsPerFrame = 0.0f;
            float m_secondsSinceLastFrame = 0.0;
            float m_playbackSpeedFactor = 1.0;
        };
    } // namespace VideoPlayback
}//namespace AZ

#endif