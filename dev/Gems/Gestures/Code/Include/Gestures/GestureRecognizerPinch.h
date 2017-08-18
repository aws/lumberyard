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
    class RecognizerPinch;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    struct IPinchListener
    {
        virtual ~IPinchListener() {}
        virtual void OnPinchInitiated(const RecognizerPinch& recognizer) {}
        virtual void OnPinchUpdated(const RecognizerPinch& recognizer) {}
        virtual void OnPinchEnded(const RecognizerPinch& recognizer) {}
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerPinch
        : public IRecognizer
    {
    public:
        inline static float GetDefaultMinPixelsMoved() { return 50.0f; }
        inline static float GetDefaultMaxAngleDegrees() { return 15.0f; }
        inline static int32_t GetDefaultPriority() { return 0; }

        struct Config
        {
            inline Config()
                : minPixelsMoved(GetDefaultMinPixelsMoved())
                , maxAngleDegrees(GetDefaultMaxAngleDegrees())
                , priority(GetDefaultPriority())
            {}

            float minPixelsMoved;
            float maxAngleDegrees;
            int32_t priority;
        };
        inline static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        inline explicit RecognizerPinch(IPinchListener& listener,
            const Config& config = GetDefaultConfig());
        inline ~RecognizerPinch() override;

        inline int32_t GetPriority() const override { return m_config.priority; }
        inline bool OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;
        inline bool OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;
        inline bool OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;

        inline Config& GetConfig() { return m_config; }
        inline const Config& GetConfig() const { return m_config; }

        inline Vec2 GetStartPosition0() const { return m_startPositions[0]; }
        inline Vec2 GetStartPosition1() const { return m_startPositions[1]; }

        inline Vec2 GetCurrentPosition0() const { return m_currentPositions[0]; }
        inline Vec2 GetCurrentPosition1() const { return m_currentPositions[1]; }

        inline Vec2 GetStartMidpoint() const { return Lerp(GetStartPosition0(), GetStartPosition1(), 0.5f); }
        inline Vec2 GetCurrentMidpoint() const { return Lerp(GetCurrentPosition0(), GetCurrentPosition1(), 0.5f); }

        inline float GetStartDistance() const { return GetStartPosition1().GetDistance(GetStartPosition0()); }
        inline float GetCurrentDistance() const { return GetCurrentPosition1().GetDistance(GetCurrentPosition0()); }
        inline float GetPinchRatio() const
        {
            const float startDistance = GetStartDistance();
            return startDistance ? GetCurrentDistance() / startDistance : 0.0f;
        }

    private:
        enum class State
        {
            Idle = -1,
            Pressed0 = 0,
            Pressed1 = 1,
            PressedBoth,
            Pinching
        };

        static const uint32_t s_maxPinchPointerIndex = 1;

        IPinchListener& m_listener;
        Config m_config;

        ScreenPosition m_startPositions[2];
        ScreenPosition m_currentPositions[2];

        int64_t m_lastUpdateTimes[2];

        State m_currentState;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerPinch.inl>
