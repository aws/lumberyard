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
    class RecognizerHold;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    struct IHoldListener
    {
        virtual ~IHoldListener() {}
        virtual void OnHoldInitiated(const RecognizerHold& recognizer) {}
        virtual void OnHoldUpdated(const RecognizerHold& recognizer) {}
        virtual void OnHoldEnded(const RecognizerHold& recognizer) {}
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerHold
        : public IRecognizer
    {
    public:
        inline static float GetDefaultMinSecondsHeld() { return 2.0f; }
        inline static float GetDefaultMaxPixelsMoved() { return 20.0f; }
        inline static uint32_t GetDefaultPointerIndex() { return 0; }
        inline static int32_t GetDefaultPriority() { return 0; }

        struct Config
        {
            inline Config()
                : minSecondsHeld(GetDefaultMinSecondsHeld())
                , maxPixelsMoved(GetDefaultMaxPixelsMoved())
                , pointerIndex(GetDefaultPointerIndex())
                , priority(GetDefaultPriority())
            {}

            float minSecondsHeld;
            float maxPixelsMoved;
            uint32_t pointerIndex;
            int32_t priority;
        };
        inline static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        inline explicit RecognizerHold(IHoldListener& listener,
            const Config& config = GetDefaultConfig());
        ~RecognizerHold() override;

        inline int32_t GetPriority() const override { return m_config.priority; }
        inline bool OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;
        inline bool OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;
        inline bool OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex) override;

        inline Config& GetConfig() { return m_config; }
        inline const Config& GetConfig() const { return m_config; }

        inline Vec2 GetStartPosition() const { return m_startPosition; }
        inline Vec2 GetCurrentPosition() const { return m_currentPosition; }

        inline float GetDuration() const { return gEnv->pTimer->GetFrameStartTime().GetDifferenceInSeconds(m_startTime); }

    private:
        enum class State
        {
            Idle,
            Pressed,
            Held
        };

        IHoldListener& m_listener;
        Config m_config;

        int64 m_startTime;
        ScreenPosition m_startPosition;
        ScreenPosition m_currentPosition;

        State m_currentState;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerHold.inl>
