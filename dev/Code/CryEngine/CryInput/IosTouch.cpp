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
#include "StdAfx.h"

#include "IosTouch.h"

#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

#import <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
CIosTouch::CIosTouch(IInput& input)
    : CMobileTouch(input, "iOS Touch")
{
    m_activeTouches.resize(AzFramework::InputDeviceTouch::Touch::All.size());

    AzFramework::RawInputNotificationBusIos::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CIosTouch::~CIosTouch()
{
    AzFramework::RawInputNotificationBusIos::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CIosTouch::OnRawTouchEventBegan(const UITouch* uiTouch)
{
    for (uint32_t i = 0; i < AzFramework::InputDeviceTouch::Touch::All.size(); ++i)
    {
        // Use the first available index.
        if (m_activeTouches[i] == nullptr)
        {
            m_activeTouches[i] = uiTouch;

            const TouchEvent::State state = TouchEvent::State::Began;
            const TouchEvent event = InitTouchEvent(uiTouch, i, state);
            OnTouchBegan(event);

            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CIosTouch::OnRawTouchEventMoved(const UITouch* uiTouch)
{
    for (uint32_t i = 0; i < AzFramework::InputDeviceTouch::Touch::All.size(); ++i)
    {
        if (m_activeTouches[i] == uiTouch)
        {
            const TouchEvent::State state = TouchEvent::State::Moved;
            const TouchEvent event = InitTouchEvent(uiTouch, i, state);
            OnTouchMoved(event);

            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CIosTouch::OnRawTouchEventEnded(const UITouch* uiTouch)
{
    for (uint32_t i = 0; i < AzFramework::InputDeviceTouch::Touch::All.size(); ++i)
    {
        if (m_activeTouches[i] == uiTouch)
        {
            const TouchEvent::State state = TouchEvent::State::Ended;
            const TouchEvent event = InitTouchEvent(uiTouch, i, state);
            OnTouchEnded(event);

            m_activeTouches[i] = nullptr;

            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
CMobileTouch::TouchEvent CIosTouch::InitTouchEvent(const UITouch* uiTouch,
                                                   uint32_t index,
                                                   TouchEvent::State state)
{
    CGPoint touchLocation = [uiTouch locationInView: uiTouch.view];
    CGSize viewSize = [uiTouch.view bounds].size;

    const float normalizedLocationX = touchLocation.x / viewSize.width;
    const float normalizedLocationY = touchLocation.y / viewSize.height;
    const float pressure = uiTouch.force / uiTouch.maximumPossibleForce;

    return TouchEvent(normalizedLocationX,
                      normalizedLocationY,
                      pressure,
                      index,
                      state);
}
