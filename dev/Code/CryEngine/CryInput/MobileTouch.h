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
#ifndef CRYINCLUDE_CRYINPUT_MOBILETOUCH_H
#define CRYINCLUDE_CRYINPUT_MOBILETOUCH_H
#pragma once

#include "InputDevice.h"

#include <AzCore/std/containers/vector.h>

#include <list>
#include <IRenderer.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class CMobileTouch
    : public CInputDevice
    , public IRenderDebugListener
{
public:
    CMobileTouch(IInput& input, const char* deviceName);
    ~CMobileTouch() override;

protected:
    int GetDeviceIndex() const override { return 0; }
    void Update(bool focus) override;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    struct TouchEvent
    {
        enum class State
        {
            Began,
            Moved,
            Ended
        };

        explicit TouchEvent(
            float a_xNormalized,
            float a_yNormalized,
            float a_pressure,
            uint32_t a_index,
            State a_state)
        : xNormalized(a_xNormalized)
        , yNormalized(a_yNormalized)
        , pressure(a_pressure)
        , index(a_index)
        , state(a_state) {}

        float       xNormalized;
        float       yNormalized;
        float       pressure;
        uint32_t    index;
        State       state;
    };

    void OnTouchBegan(const TouchEvent& touchEvent);
    void OnTouchMoved(const TouchEvent& touchEvent);
    void OnTouchEnded(const TouchEvent& touchEvent);
    void ProcessTouchEvent(const TouchEvent& touchEvent);

    void SimulateMouseEvent(const TouchEvent& touchEvent);
    void SimulateMouseEvent(EKeyId keyId,
                            EInputState inputState,
                            float screenPositionX,
                            float screenPositionY,
                            float value);

    void OnDebugDraw() override;

private:
    AZStd::vector<TouchEvent> m_filteredTouchEvents;

    // For simulating the mouse with the primary touch.
    float m_mousePositionX;
    float m_mousePositionY;

#if !defined(_RELEASE)
    std::map<int32, Vec2> m_debugTouchesToDraw; // Map of touches to debug draw, keyed by finger id
#endif // !defined(_RELEASE)
};

#endif // CRYINCLUDE_CRYINPUT_MOBILETOUCH_H
