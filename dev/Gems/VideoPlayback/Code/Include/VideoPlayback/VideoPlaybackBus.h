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
#include <AzCore/Component/EntityId.h>

namespace AZ 
{
    namespace VideoPlayback
    {
        class VideoPlaybackRequests
            : public AZ::EBusTraits
        {

        public:

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef AZ::EntityId BusIdType;
            //////////////////////////////////////////////////////////////////////////

            VideoPlaybackRequests() = default;
            virtual ~VideoPlaybackRequests() = default;

            /*
            * Start/resume playing a movie that is attached to the current entity.
            */
            virtual void Play() = 0;

            /*
            * Pause a movie that is attached to the current entity.
            */
            virtual void Pause() = 0;

            /*
            * Stop playing a movie that is attached to the current entity.
            */
            virtual void Stop() = 0;

            /*
            * Set whether or not the movie attached to the current entity should loop.
            */
            virtual void EnableLooping(bool enable) = 0;

            /*
            * Returns whether or not the video is currently playing
            * 
            * @return True if the video is currently playing. False if the video is paused or stopped
            */
            virtual bool IsPlaying() = 0;

            /* Sets the playback speed based on a factor of the current playback speed
            *
            * For example you can play at half speed by passing 0.5f or play at double speed by
            * passing 2.0f. 
            *
            * @param speedFactor The speed modification factor to apply to playback speed
            */
            virtual void SetPlaybackSpeed(float speedFactor) = 0;
        };
        using VideoPlaybackRequestBus = AZ::EBus<VideoPlaybackRequests>;


        class VideoPlaybackNotifications
            : public AZ::EBusTraits
        {

        public:

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef AZ::EntityId BusIdType;
            //////////////////////////////////////////////////////////////////////////

            VideoPlaybackNotifications() = default;
            virtual ~VideoPlaybackNotifications() = default;

            /*
            * Event that fires when the movie starts playback.
            */
            virtual void OnPlaybackStarted() = 0;

            /*
            * Event that fires when the movie pauses playback.
            */
            virtual void OnPlaybackPaused() = 0;

            /*
            * Event that fires when the movie stops playback.
            */
            virtual void OnPlaybackStopped() = 0;

            /*
            * Event that fires when the movie completes playback.
            */
            virtual void OnPlaybackFinished() = 0;
        };
        using VideoPlaybackNotificationBus = AZ::EBus<VideoPlaybackNotifications>;
    } // namespace VideoPlayback
}//namespace AZ