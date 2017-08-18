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
    class RecognizerSwipe;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    struct ISwipeListener
    {
        virtual ~ISwipeListener() {}
        virtual void OnSwipeRecognized(const RecognizerSwipe& recognizer) = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerSwipe
        : public IRecognizer
    {
    public:
        inline static float GetDefaultMaxSecondsHeld() { return 0.5f; }
        inline static float GetDefaultMinPixelsMoved() { return 100.0f; }
        inline static uint32_t GetDefaultPointerIndex() { return 0; }
        inline static int32_t GetDefaultPriority() { return 0; }

        struct Config
        {
            inline Config()
                : maxSecondsHeld(GetDefaultMaxSecondsHeld())
                , minPixelsMoved(GetDefaultMinPixelsMoved())
                , pointerIndex(GetDefaultPointerIndex())
                , priority(GetDefaultPriority())
            {}

            float maxSecondsHeld;
            float minPixelsMoved;
            uint32_t pointerIndex;
            int32_t priority;
        };
        inline static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        inline explicit RecognizerSwipe(ISwipeListener& listener,
            const Config& config = GetDefaultConfig());
        inline ~RecognizerSwipe() override;

        inline int32_t GetPriority() const override { return m_config.priority; }
        inline bool OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;
        inline bool OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;
        inline bool OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;

        inline Config& GetConfig() { return m_config; }
        inline const Config& GetConfig() const { return m_config; }

        inline Vec2 GetStartPosition() const { return m_startPosition; }
        inline Vec2 GetEndPosition() const { return m_endPosition; }

        inline Vec2 GetDelta() const { return GetEndPosition() - GetStartPosition(); }
        inline Vec2 GetDirection() const { return GetDelta().GetNormalized(); }

        inline float GetDistance() const { return GetEndPosition().GetDistance(GetStartPosition()); }
        inline float GetDuration() const { return CTimeValue(m_endTime).GetDifferenceInSeconds(m_startTime); }
        inline float GetVelocity() const { return GetDistance() / GetDuration(); }

    private:
        enum class State
        {
            Idle,
            Pressed
        };

        ISwipeListener& m_listener;
        Config m_config;

        ScreenPosition m_startPosition;
        ScreenPosition m_endPosition;

        int64 m_startTime;
        int64 m_endTime;

        State m_currentState;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerSwipe.inl>
