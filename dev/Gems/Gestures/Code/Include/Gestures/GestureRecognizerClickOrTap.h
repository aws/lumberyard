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

#include "IGestureRecognizer.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    class RecognizerClickOrTap;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    struct IClickOrTapListener
    {
        virtual ~IClickOrTapListener() {}
        virtual void OnClickOrTapRecognized(const RecognizerClickOrTap& recognizer) = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerClickOrTap
        : public IRecognizer
    {
    public:
        inline static float GetDefaultMaxSecondsHeld() { return 0.5f; }
        inline static float GetDefaultMaxPixelsMoved() { return 20.0f; }
        inline static float GetDefaultMaxSecondsBetweenClicksOrTaps() { return 0.5f; }
        inline static float GetDefaultMaxPixelsBetweenClicksOrTaps() { return 20.0f; }
        inline static uint32_t GetDefaultMinClicksOrTaps() { return 1; }
        inline static uint32_t GetDefaultPointerIndex() { return 0; }
        inline static int32_t GetDefaultPriority() { return 0; }

        struct Config
        {
            inline Config()
                : maxSecondsHeld(GetDefaultMaxSecondsHeld())
                , maxPixelsMoved(GetDefaultMaxPixelsMoved())
                , maxSecondsBetweenClicksOrTaps(GetDefaultMaxSecondsBetweenClicksOrTaps())
                , maxPixelsBetweenClicksOrTaps(GetDefaultMaxPixelsBetweenClicksOrTaps())
                , minClicksOrTaps(GetDefaultMinClicksOrTaps())
                , pointerIndex(GetDefaultPointerIndex())
                , priority(GetDefaultPriority())
            {}

            float maxSecondsHeld;
            float maxPixelsMoved;
            float maxSecondsBetweenClicksOrTaps;
            float maxPixelsBetweenClicksOrTaps;
            uint32_t minClicksOrTaps;
            uint32_t pointerIndex;
            int32_t priority;
        };
        inline static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        inline explicit RecognizerClickOrTap(IClickOrTapListener& listener,
            const Config& config = GetDefaultConfig());
        inline ~RecognizerClickOrTap() override;

        inline int32_t GetPriority() const override { return m_config.priority; }
        inline bool OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;
        inline bool OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;
        inline bool OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;

        inline Config& GetConfig() { return m_config; }
        inline const Config& GetConfig() const { return m_config; }

        inline Vec2 GetStartPosition() const { return m_positionOfFirstEvent; }
        inline Vec2 GetEndPosition() const { return m_positionOfLastEvent; }

    private:
        enum class State
        {
            Idle,
            Pressed,
            Released
        };

        IClickOrTapListener& m_listener;
        Config m_config;

        int64 m_timeOfLastEvent;
        ScreenPosition m_positionOfFirstEvent;
        ScreenPosition m_positionOfLastEvent;

        uint32_t m_currentCount;
        State m_currentState;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerClickOrTap.inl>
